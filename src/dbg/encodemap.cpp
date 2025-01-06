#include "encodemap.h"
#include <unordered_map>
#include "addrinfo.h"
#include <zydis_wrapper.h>

struct ENCODEMAP : AddrInfo
{
    duint size;
    uint8_t* data;
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
        if(!StringUtils::FromCompressedHex(json_string_value(dataJson), data))
            return false;
        value.size = data.size();
        value.data = (uint8_t*)VirtualAlloc(NULL, value.size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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

static bool EncodeMapValidateModuleInfo(duint key, ENCODEMAP & map, duint segsize)
{
    // The map size might be smaller if it was loaded from cache for another
    // version of the module. Adjust the size in this case.
    if(map.size < segsize)
    {
        auto newData = (uint8_t*)VirtualAlloc(NULL, segsize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(newData == NULL) return false;

        memcpy(newData, map.data, map.size);

        DecreaseReferenceCount(map.data);
        VirtualFree(map.data, 0, MEM_RELEASE);
        encmaps.Delete(key);

        map.size = segsize;
        map.data = newData;
        IncreaseReferenceCount(map.data);
        encmaps.Add(map);
    }

    return true;
}

static bool EncodeMapGet(duint addr, ENCODEMAP & map, duint* baseOut = nullptr)
{
    duint base, segsize;

    base = MemFindBaseAddr(addr, &segsize);
    if(!base)
        return false;

    if(baseOut)
        *baseOut = base;

    duint key = EncodeMap::VaKey(base);
    if(!encmaps.Get(key, map))
        return false;

    if(!EncodeMapValidateModuleInfo(key, map, segsize))
        return false;

    return true;
}

static bool EncodeMapGetorCreate(duint addr, ENCODEMAP & map, duint* baseOut = nullptr, bool* created = nullptr)
{
    duint base, segsize;

    base = MemFindBaseAddr(addr, &segsize);
    if(!base)
        return false;

    if(baseOut)
        *baseOut = base;

    duint key = EncodeMap::VaKey(base);
    if(!encmaps.Get(key, map))
    {
        if(created)
            *created = true;
        map.size = segsize;
        map.data = (uint8_t*)VirtualAlloc(NULL, segsize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if(map.data == NULL) return false;
        IncreaseReferenceCount(map.data);
        encmaps.PrepareValue(map, base, false);
        encmaps.Add(map);
    }
    else
    {
        if(created)
            *created = false;
        if(!EncodeMapValidateModuleInfo(key, map, segsize))
            return false;
    }
    return true;
}

void* EncodeMapGetBuffer(duint addr, duint* size, bool create)
{
    ENCODEMAP map;
    duint base;
    if(create ? EncodeMapGetorCreate(addr, map, &base) : EncodeMapGet(addr, map, &base))
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
    ENCODEMAP map;
    duint base;
    if(EncodeMapGet(addr, map, &base))
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
    ENCODEMAP map;
    duint base;
    if(EncodeMapGet(addr, map, &base))
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
    ENCODEMAP map;
    duint base;
    if(!EncodeMapGetorCreate(addr, map, &base, created))
        return false;
    auto offset = addr - base;
    size = std::min(map.size - offset, size);
    auto datasize = GetEncodeTypeSize(type);
    if(datasize == 1 && !IsCodeType(type))
    {
        memset(map.data + offset, (uint8_t)type, size);
    }
    else
    {
        memset(map.data + offset, (uint8_t)enc_middle, size);
        if(IsCodeType(type) && size > 1)
        {
            Zydis zydis;
            Memory<unsigned char*> buffer(size);
            if(!MemRead(addr, buffer(), size))
                return false;

            duint buffersize = size, bufferoffset = 0, cmdsize;
            for(auto i = offset; i < offset + size;)
            {
                map.data[i] = (uint8_t)type;
                zydis.Disassemble(base + i, buffer() + bufferoffset, int(buffersize - bufferoffset));
                cmdsize = zydis.Success() ? zydis.Size() : 1;
                i += cmdsize;
                bufferoffset += cmdsize;
                buffersize -= cmdsize;
            }
        }
        else
        {
            for(auto i = offset; i < offset + size; i += datasize)
                map.data[i] = (uint8_t)type;
        }

    }

    for(auto i = offset + size + 1; i < map.size; i++)
    {
        if(map.data[i] == enc_middle)
            map.data[i] = (uint8_t)enc_unknown;
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
