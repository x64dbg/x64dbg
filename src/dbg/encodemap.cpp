#include "encodemap.h"
#include <unordered_map>
#include "addrinfo.h"
#include <algorithm>
#include <capstone_wrapper.h>

struct ENCODEMAP : AddrInfo
{
    duint size;
    byte* data;
};

std::unordered_map<duint, duint> referenceCount;

void IncreaseReferenceCount(void* buffer, bool lock = true)
{
    if(lock)
    {
        EXCLUSIVE_ACQUIRE(LockEncodeMaps);
    }
    auto iter = referenceCount.find((duint)buffer);
    if(iter == referenceCount.end())
        referenceCount[(duint)buffer] = 1;
    else
        referenceCount[(duint)buffer]++;
}

duint DecreaseReferenceCount(void* buffer, bool lock = true)
{
    if(lock)
    {
        EXCLUSIVE_ACQUIRE(LockEncodeMaps);
    }
    auto iter = referenceCount.find((duint)buffer);
    if(iter == referenceCount.end())
        return -1;
    if(iter->second == 1)
    {
        referenceCount.erase(iter->first);
        return 0;
    }
    else
        referenceCount[iter->first]--;
    return iter->second;
}

struct EncodeMapSerializer : AddrInfoSerializer<ENCODEMAP>
{
    bool Save(const ENCODEMAP & value) override
    {

        AddrInfoSerializer::Save(value);
        set("data", json_stringn((const char*)value.data, value.size));
        return true;
    }

    bool Load(ENCODEMAP & value) override
    {
        if(!AddrInfoSerializer::Load(value))
            return false;
        auto data = get("data");
        value.size = json_string_length(data);
        value.data = (byte*)VirtualAlloc(NULL, value.size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(value.data == NULL) return false;
        memcpy(value.data, (byte*)json_string_value(data), value.size);
        IncreaseReferenceCount(value.data, false);
        return true;
    }
};

struct EncodeMap : AddrInfoHashMap<LockEncodeMaps, ENCODEMAP, EncodeMapSerializer>
{
    const char* jsonKey() const override
    {
        return "encodemaps";
    }
};

static EncodeMap encmaps;

bool EncodeMapGetorCreate(duint addr, ENCODEMAP & map)
{
    duint base, segsize;

    base = MemFindBaseAddr(addr, &segsize);
    if(!base)
        return false;

    duint key = EncodeMap::VaKey(base);
    if(!encmaps.Contains(key))
    {
        map.size = segsize;
        map.data = (byte*)VirtualAlloc(NULL, segsize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(map.data == NULL) return false;
        IncreaseReferenceCount(map.data);
        encmaps.PrepareValue(map, base, false);
        encmaps.Add(map);
    }
    else
    {
        if(!encmaps.Get(key, map))
            return false;
    }
    return true;
}

void* EncodeMapGetBuffer(duint addr, bool create)
{
    duint base, size;

    if(!MemIsValidReadPtr(addr))
        return nullptr;
    base = MemFindBaseAddr(addr, &size);

    ENCODEMAP map;
    bool result = create ? EncodeMapGetorCreate(addr, map) : encmaps.Get(EncodeMap::VaKey(base), map);
    if(result)
    {
        duint offset = addr - base;
        if(offset >= map.size)
            return nullptr;
        IncreaseReferenceCount(map.data);
        return map.data;
    }
    return nullptr;
}

void EncodeMapReleaseBuffer(void* buffer, bool lock)
{
    if(DecreaseReferenceCount(buffer, lock) == 0)
        VirtualFree(buffer, 0, MEM_RELEASE);
}

void EncodeMapReleaseBuffer(void* buffer)
{
    EncodeMapReleaseBuffer(buffer, true);
}

bool IsRangeConflict(byte* typebuffer, duint size, duint codesize)
{
    if(codesize > size)
        return true;
    size = min(size, codesize);
    if(size <= 0)
        return false;
    ENCODETYPE type = (ENCODETYPE)typebuffer[0];
    if(type == enc_middle)
        return true;
    for(int i = 1; i < size; i++)
    {
        if((ENCODETYPE)typebuffer[i] != enc_unknown && (ENCODETYPE)typebuffer[i] != enc_middle)
            return true;
    }

    return false;
}

duint GetEncodeTypeSize(ENCODETYPE type)
{
    switch(type)
    {
    case enc_byte:
        return 1;
    case enc_word:
        return 2;
    case enc_dword:
        return 4;
    case enc_fword:
        return 6;
    case enc_qword:
        return 8;
    case enc_tbyte:
        return 10;
    case enc_oword:
        return 16;
    case enc_mmword:
        return 8;
    case enc_xmmword:
        return 16;
    case enc_ymmword:
        return 32;
    case enc_real4:
        return 4;
    case enc_real8:
        return 8;
    case enc_real10:
        return 10;
    case enc_ascii:
        return 1;
    case enc_unicode:
        return 2;
    default:
        return 1;
    }
}

bool IsCodeType(ENCODETYPE type)
{
    return type == enc_code || type == enc_junk;
}

ENCODETYPE EncodeMapGetType(duint addr, duint codesize)
{
    duint base, size;

    base = MemFindBaseAddr(addr, &size);
    if(!base)
        return ENCODETYPE::enc_unknown;

    ENCODEMAP map;
    if(encmaps.Get(EncodeMap::VaKey(base), map))
    {
        duint offset = addr - base;
        if(offset >= map.size)
            return ENCODETYPE::enc_unknown;
        ENCODETYPE type = (ENCODETYPE)map.data[offset];

        if((type == enc_unknown || type == enc_middle) && IsRangeConflict(map.data + offset, size - offset, codesize))
            return enc_byte;
        else
            return type;
    }

    return ENCODETYPE::enc_unknown;
}

duint EncodeMapGetSize(duint addr, duint codesize)
{
    duint base;

    base = MemFindBaseAddr(addr, 0);
    if(!base)
        return codesize;

    ENCODEMAP map;
    if(encmaps.Get(EncodeMap::VaKey(base), map))
    {
        duint offset = addr - base;
        if(offset >= map.size)
            return 1;
        ENCODETYPE type = (ENCODETYPE)map.data[offset];

        duint datasize = GetEncodeTypeSize(type);
        if(type == enc_unknown || type == enc_code || type == enc_junk)
        {
            if(IsRangeConflict(map.data + offset, map.size - offset, codesize) || codesize == 0)
                return datasize;
            else
                return codesize;
        }
        else if(type == enc_ascii || type == enc_unicode)
        {
            duint totalsize = 0;
            for(auto i = offset; i < map.size; i += datasize)
            {
                if(map.data[i] == type)
                    totalsize += datasize;
                else
                    break;
            }
            return totalsize;
        }
        else
            return datasize;
    }

    return codesize;
}

bool EncodeMapSetType(duint addr, duint size, ENCODETYPE type)
{
    duint base;

    base = MemFindBaseAddr(addr, 0);
    if(!base)
        return false;


    ENCODEMAP map;
    if(!EncodeMapGetorCreate(base, map))
        return false;
    duint offset = addr - base;
    size = min(map.size - offset, size);
    duint datasize = GetEncodeTypeSize(type);
    if(datasize == 1 && !IsCodeType(type))
    {
        memset(map.data + offset, (byte)type, size);
    }
    else
    {
        memset(map.data + offset, (byte)enc_middle, size);
        if(IsCodeType(type) && size > 1)
        {
            Capstone cp;
            unsigned char* buffer = new unsigned char[size];
            if(!MemRead(addr, buffer, size))
            {
                delete[] buffer;
                return false;

            }
            duint buffersize = size, bufferoffset = 0, cmdsize;
            for(auto i = offset; i < offset + size;)
            {
                map.data[i] = (byte)type;
                cp.Disassemble(base + i, buffer + bufferoffset, int(buffersize));
                cmdsize = cp.Success() ? cp.Size() : 1;
                i += cmdsize;
                bufferoffset += cmdsize;
                buffersize -= cmdsize;
            }

        }
        else
        {
            for(auto i = offset; i < offset + size; i += datasize)
                map.data[i] = (byte)type;
        }

    }

    for(auto i = offset + size + 1; i < map.size; i++)
    {
        if(map.data[i] == enc_middle)
            map.data[i] = (byte)enc_unknown;
        else
            break;
    }
    return true;
}

void EncodeMapDelSegment(duint Start)
{
    duint base = MemFindBaseAddr(Start, 0);
    if(!base)
        return;
    duint key = EncodeMap::VaKey(base);
    ENCODEMAP map;
    if(encmaps.Contains(key))
    {
        encmaps.Get(key, map);
        EncodeMapReleaseBuffer(map.data);
    }
    encmaps.Delete(key);
}

void EncodeMapDelRange(duint Start, duint End)
{
    EncodeMapSetType(Start, End - Start + 1, enc_unknown);
}

void EncodeMapCacheSave(JSON Root)
{
    encmaps.CacheSave(Root);
}

void EncodeMapCacheLoad(JSON Root)
{
    encmaps.CacheLoad(Root);
}

void EncodeMapClear()
{
    EXCLUSIVE_ACQUIRE(LockEncodeMaps);
    for(auto & encmap : encmaps.GetDataUnsafe())
        EncodeMapReleaseBuffer(encmap.second.data, false);
    EXCLUSIVE_RELEASE();
    encmaps.Clear();
}
