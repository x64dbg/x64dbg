#include "encodemap.h"
#include <unordered_map>
#include "addrinfo.h"
#include "serializablemap.h"
#include <zydis_wrapper.h>

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
        setString("data", StringUtils::ToCompressedHex(value.data, value.size));
        return true;
    }

    bool Load(ENCODEMAP & value) override
    {
        if(!AddrInfoSerializer::Load(value))
            return false;
        auto dataJson = get("data");
        if(!dataJson)
            return false;
        std::vector<unsigned char> data;
        if(!StringUtils::FromCompressedHex(dataJson->GetString(), data))
            return false;
        value.size = data.size();
        value.data = (byte*)VirtualAlloc(NULL, value.size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(!value.data)
            return false;
        memcpy(value.data, data.data(), data.size());
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

static bool EncodeMapGetorCreate(duint addr, ENCODEMAP & map, bool* created = nullptr)
{
    duint base, segsize;

    base = MemFindBaseAddr(addr, &segsize);
    if(!base)
        return false;

    duint key = EncodeMap::VaKey(base);
    if(!encmaps.Contains(key))
    {
        if(created)
            *created = true;
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

void* EncodeMapGetBuffer(duint addr, duint* size, bool create)
{
    auto base = MemFindBaseAddr(addr);

    ENCODEMAP map;
    if(create ? EncodeMapGetorCreate(addr, map) : encmaps.Get(EncodeMap::VaKey(base), map))
    {
        auto offset = addr - base;
        if(offset < map.size)
        {
            IncreaseReferenceCount(map.data);
            if(size)
                *size = map.size;
            return map.data;
        }
    }
    if(size)
        *size = 0;
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

static bool IsCodeType(ENCODETYPE type)
{
    return type == enc_code || type == enc_junk;
}

ENCODETYPE EncodeMapGetType(duint addr, duint codesize)
{
    duint size;
    auto base = MemFindBaseAddr(addr, &size);

    ENCODEMAP map;
    if(base && encmaps.Get(EncodeMap::VaKey(base), map))
    {
        auto offset = addr - base;
        if(offset >= map.size)
            return enc_unknown;
        return ENCODETYPE(map.data[offset]);
    }

    return enc_unknown;
}

duint EncodeMapGetSize(duint addr, duint codesize)
{
    auto base = MemFindBaseAddr(addr, nullptr);
    if(!base)
        return codesize;

    ENCODEMAP map;
    if(encmaps.Get(EncodeMap::VaKey(base), map))
    {
        auto offset = addr - base;
        if(offset >= map.size)
            return 1;
        auto type = ENCODETYPE(map.data[offset]);

        auto datasize = GetEncodeTypeSize(type);
        if(!IsCodeType(type))
            return datasize;
    }

    return codesize;
}

bool EncodeMapSetType(duint addr, duint size, ENCODETYPE type, bool* created)
{
    auto base = MemFindBaseAddr(addr, nullptr);
    if(!base)
        return false;

    ENCODEMAP map;
    if(created)
        *created = false;
    if(!EncodeMapGetorCreate(base, map, created))
        return false;
    auto offset = addr - base;
    size = min(map.size - offset, size);
    auto datasize = GetEncodeTypeSize(type);
    if(datasize == 1 && !IsCodeType(type))
    {
        memset(map.data + offset, (byte)type, size);
    }
    else
    {
        memset(map.data + offset, (byte)enc_middle, size);
        if(IsCodeType(type) && size > 1)
        {
            Zydis cp;
            Memory<unsigned char*> buffer(size);
            if(!MemRead(addr, buffer(), size))
                return false;

            duint buffersize = size, bufferoffset = 0, cmdsize;
            for(auto i = offset; i < offset + size;)
            {
                map.data[i] = (byte)type;
                cp.Disassemble(base + i, buffer() + bufferoffset, int(buffersize - bufferoffset));
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

void EncodeMapCacheSave(rapidjson::Document & Root)
{
    encmaps.CacheSave(Root);
}

void EncodeMapCacheLoad(rapidjson::Document & Root)
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
