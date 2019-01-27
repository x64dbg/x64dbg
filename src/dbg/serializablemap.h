#ifndef _SERIALIZABLEMAP_H
#define _SERIALIZABLEMAP_H

#include "_global.h"
#include "threading.h"
#include "module.h"
#include "memory.h"
#include "jansson/jansson_x64dbg.h"
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#undef RAPIDJSON_HAS_STDSTRING

template<class TValue>
class JSONWrapper
{
public:
    virtual ~JSONWrapper()
    {
    }

    void SetJson(const rapidjson::Value* value, rapidjson::Document::AllocatorType* allocator)
    {
        mValue = value;
        mAllocator = allocator;
    }

    virtual bool Save(const TValue & value) = 0;
    virtual bool Load(TValue & value) = 0;

protected:
    void setString(const char* key, const std::string & value)
    {
        mJson->AddMember(rapidjson::Value(key, *mAllocator), rapidjson::Value(value, *mAllocator), *mAllocator);
    }

    bool getString(const char* key, std::string & dest) const
    {
        auto jsonValue = get(key);
        if(!jsonValue)
            return false;
        if(!jsonValue->IsString())
            return false;
        dest = jsonValue->GetString();
        return true;
    }

    void setHex(const char* key, duint value)
    {
        char hexvalue[20];
        sprintf_s(hexvalue, "0x%llX", value);
        setString(key, hexvalue);
    }

    bool getHex(const char* key, duint & value) const
    {
        std::string hex;
        if(!getString(key, hex))
            return false;
        unsigned json_int_t ret = 0;
        if(sscanf_s(hex.c_str(), "0x%llX", &ret) != 1)
            return false;
        value = duint(ret);
        return true;
    }

    void setBool(const char* key, bool value)
    {
        set(key, rapidjson::Value(value, *mAllocator));
    }

    bool getBool(const char* key, bool & value) const
    {
        auto jsonValue = get(key);
        if(!jsonValue)
            return false;
        if(!jsonValue->IsBool())
            return false;
        value = jsonValue->GetBool();
        return true;
    }

    template<typename T>
    void setInt(const char* key, T value)
    {
        set(key, rapidjson::Value(value, *mAllocator));
    }

    template<typename T>
    bool getInt(const char* key, T & value)
    {
        auto jsonValue = get(key);
        if(!jsonValue)
            return false;
        if(!jsonValue->Is<T>())
            return false;
        value = jsonValue->Get<T>();
        return true;
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void set(const char* key, const rapidjson::Value & value)
    {
        mJson->AddMember(rapidjson::Value(key, *mAllocator), value);
    }

    const rapidjson::Value* get(const char* key) const
    {
        auto itr = mJson->FindMember(key);
        return itr == mJson->MemberEnd() ? nullptr : &itr->value;
    }

    rapidjson::Value* mJson = nullptr;
    rapidjson::Document::AllocatorType* mAllocator = nullptr;
};

template<SectionLock TLock, class TKey, class TValue, class TMap, class TSerializer>
class SerializableTMap
{
    static_assert(std::is_base_of<JSONWrapper<TValue>, TSerializer>::value, "TSerializer is not derived from JSONWrapper<TValue>");
public:
    using TValuePred = std::function<bool(const TValue & value)>;

    virtual ~SerializableTMap()
    {
    }

    bool Add(const TValue & value)
    {
        EXCLUSIVE_ACQUIRE(TLock);
        return addNoLock(value);
    }

    bool Get(const TKey & key, TValue & value) const
    {
        SHARED_ACQUIRE(TLock);
        auto found = mMap.find(key);
        if(found == mMap.end())
            return false;
        value = found->second;
        return true;
    }

    bool Contains(const TKey & key) const
    {
        SHARED_ACQUIRE(TLock);
        return mMap.count(key) > 0;
    }

    bool Delete(const TKey & key)
    {
        EXCLUSIVE_ACQUIRE(TLock);
        return mMap.erase(key) > 0;
    }

    void DeleteWhere(TValuePred predicate)
    {
        EXCLUSIVE_ACQUIRE(TLock);
        for(auto itr = mMap.begin(); itr != mMap.end();)
        {
            if(predicate(itr->second))
                itr = mMap.erase(itr);
            else
                ++itr;
        }
    }

    bool GetWhere(TValuePred predicate, TValue & value)
    {
        return getWhere(predicate, &value);
    }

    bool GetWhere(TValuePred predicate)
    {
        return getWhere(predicate, nullptr);
    }

    void Clear()
    {
        EXCLUSIVE_ACQUIRE(TLock);
        TMap empty;
        std::swap(mMap, empty);
    }

    void CacheSave(const rapidjson::Document & root) const
    {
        SHARED_ACQUIRE(TLock);
        auto jsonValues = rapidjson::Value(rapidjson::kArrayType, root.GetAllocator());
        TSerializer serializer;
        for(const auto & itr : mMap)
        {
            rapidjson::Value jsonValue(rapidjson::kObjectType, root.GetAllocator());
            serializer.SetJson(jsonValue, root.GetAllocator());
            if(serializer.Save(itr.second))
                jsonValues.PushBack(jsonValue);
        }
        if(jsonValues.Size())
            root.AddMember(rapidjson::Value(jsonKey(), root.GetAllocator()), jsonValues, root.GetAllocator());
    }

    void CacheLoad(const rapidjson::Document & root, const char* keyprefix = nullptr)
    {
        EXCLUSIVE_ACQUIRE(TLock);
        String key = keyprefix ? (keyprefix + String(jsonKey())) : jsonKey();
        auto itr = root.FindMember(key);
        if(itr == root.MemberEnd())
            return;
        const rapidjson::Value & jsonValues = itr->value;
        if(!itr->value.IsArray())
            return;
        TSerializer deserializer;
        for(size_t i = 0; i < jsonValues.Size(); i++)
        {
            deserializer.SetJson(jsonValues[i]);
            TValue value;
            if(deserializer.Load(value))
                addNoLock(value);
        }
    }

    void GetList(std::vector<TValue> & values) const
    {
        SHARED_ACQUIRE(TLock);
        values.clear();
        values.reserve(mMap.size());
        for(const auto & itr : mMap)
            values.push_back(itr.second);
    }

    bool Enum(TValue* list, size_t* size) const
    {
        if(!list && !size)
            return false;
        SHARED_ACQUIRE(TLock);
        if(size)
        {
            *size = mMap.size() * sizeof(TValue);
            if(!list)
                return true;
        }
        for(auto & itr : mMap)
        {
            *list = itr.second;
            AdjustValue(*list);
            ++list;
        }
        return true;
    }

    bool GetInfo(const TKey & key, TValue* valuePtr) const
    {
        TValue value;
        if(!Get(key, value))
            return false;
        if(valuePtr)
            *valuePtr = value;
        return true;
    }

    TMap & GetDataUnsafe()
    {
        return mMap;
    }

    virtual void AdjustValue(TValue & value) const = 0;

protected:
    virtual const char* jsonKey() const = 0;
    virtual TKey makeKey(const TValue & value) const = 0;

private:
    TMap mMap;

    bool addNoLock(const TValue & value)
    {
        mMap[makeKey(value)] = value;
        return true;
    }

    bool getWhere(TValuePred predicate, TValue* value)
    {
        SHARED_ACQUIRE(TLock);
        for(const auto & itr : mMap)
        {
            if(!predicate(itr.second))
                continue;
            if(value)
                *value = itr.second;
            return true;
        }
        return false;
    }
};

template<SectionLock TLock, class TKey, class TValue, class TSerializer, class THash = std::hash<TKey>>
using SerializableUnorderedMap = SerializableTMap<TLock, TKey, TValue, std::unordered_map<TKey, TValue, THash>, TSerializer>;

template<SectionLock TLock, class TKey, class TValue, class TSerializer, class TCompare = std::less<TKey>>
using SerializableMap = SerializableTMap<TLock, TKey, TValue, std::map<TKey, TValue, TCompare>, TSerializer>;

template<SectionLock TLock, class TValue, class TSerializer>
struct SerializableModuleRangeMap : SerializableMap<TLock, ModuleRange, TValue, TSerializer, ModuleRangeCompare>
{
    static ModuleRange VaKey(duint start, duint end)
    {
        auto moduleBase = ModBaseFromAddr(start);
        return ModuleRange(ModHashFromAddr(moduleBase), Range(start - moduleBase, end - moduleBase));
    }
};

template<SectionLock TLock, class TValue, class TSerializer>
struct SerializableModuleHashMap : SerializableUnorderedMap<TLock, duint, TValue, TSerializer>
{
    static duint VaKey(duint addr)
    {
        return ModHashFromAddr(addr);
    }

    void DeleteRangeWhere(duint start, duint end, std::function<bool(duint, duint, const TValue &)> inRange)
    {
        // Are all comments going to be deleted?
        // 0x00000000 - 0xFFFFFFFF
        if(start == 0 && end == ~0)
        {
            this->Clear();
        }
        else
        {
            // Make sure 'Start' and 'End' reference the same module
            duint moduleBase = ModBaseFromAddr(start);

            if(moduleBase != ModBaseFromAddr(end))
                return;

            // Virtual -> relative offset
            start -= moduleBase;
            end -= moduleBase;

            this->DeleteWhere([start, end, inRange](const TValue & value)
            {
                return inRange(start, end, value);
            });
        }
    }
};

template<class TValue>
struct AddrInfoSerializer : JSONWrapper<TValue>
{
    static_assert(std::is_base_of<AddrInfo, TValue>::value, "TValue is not derived from AddrInfo");

    bool Save(const TValue & value) override
    {
        this->setString("module", value.mod());
        this->setHex("address", value.addr);
        this->setBool("manual", value.manual);
        return true;
    }

    bool Load(TValue & value) override
    {
        value.manual = true; //legacy support
        this->getBool("manual", value.manual);
        std::string mod;
        if(!this->getString("module", mod))
            return false;
        value.modhash = ModHashFromName(mod.c_str());
        return this->getHex("address", value.addr);
    }
};

template<SectionLock TLock, class TValue, class TSerializer>
struct AddrInfoHashMap : SerializableModuleHashMap<TLock, TValue, TSerializer>
{
    static_assert(std::is_base_of<AddrInfo, TValue>::value, "TValue is not derived from AddrInfo");
    static_assert(std::is_base_of<AddrInfoSerializer<TValue>, TSerializer>::value, "TSerializer is not derived from AddrInfoSerializer");

    void AdjustValue(TValue & value) const override
    {
        value.addr += ModBaseFromName(value.mod().c_str());
    }

    bool PrepareValue(TValue & value, duint addr, bool manual)
    {
        if(!MemIsValidReadPtr(addr))
            return false;
        auto base = ModBaseFromAddr(addr);
        value.modhash = ModHashFromAddr(base);
        value.manual = manual;
        value.addr = addr - base;
        return true;
    }

    void DeleteRange(duint start, duint end, bool manual)
    {
        this->DeleteRangeWhere(start, end, [manual](duint start, duint end, const TValue & value)
        {
            if(manual ? !value.manual : value.manual) //ignore non-matching entries
                return false;
            return value.addr >= start && value.addr < end;
        });
    }

protected:
    duint makeKey(const TValue & value) const override
    {
        return value.modhash + value.addr;
    }
};

#endif // _SERIALIZABLEMAP_H