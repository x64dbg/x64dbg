#ifndef _SERIALIZABLEMAP_H
#define _SERIALIZABLEMAP_H

#include "_global.h"
#include "_plugins.h"
#include "threading.h"
#include "module.h"
#include "memory.h"
#include "jansson/jansson_x64dbg.h"
#include "database_cb_batcher.h"

template<class TValue>
class JSONWrapper
{
public:
    virtual ~JSONWrapper() = default;

    void SetJson(JSON json)
    {
        mJson = json;
    }

    virtual bool Save(const TValue & value) = 0;
    virtual bool Load(TValue & value) = 0;

protected:
    void setString(const char* key, const std::string & value)
    {
        set(key, json_string(value.c_str()));
    }

    bool getString(const char* key, std::string & dest) const
    {
        auto jsonValue = get(key);
        if(!jsonValue)
            return false;
        auto str = json_string_value(jsonValue);
        if(!str)
            return false;
        dest = str;
        return true;
    }

    void setHex(const char* key, duint value)
    {
        set(key, json_hex(value));
    }

    bool getHex(const char* key, duint & value) const
    {
        auto jsonValue = get(key);
        if(!jsonValue)
            return false;
        value = duint(json_hex_value(jsonValue));
        return true;
    }

    void setBool(const char* key, bool value)
    {
        set(key, json_boolean(value));
    }

    bool getBool(const char* key, bool & value) const
    {
        auto jsonValue = get(key);
        if(!jsonValue)
            return false;
        value = json_boolean_value(jsonValue);
        return true;
    }

    template<typename T>
    void setInt(const char* key, T value)
    {
        set(key, json_integer(value));
    }

    template<typename T>
    bool getInt(const char* key, T & value)
    {
        auto jsonValue = get(key);
        if(!jsonValue)
            return false;
        value = T(json_integer_value(jsonValue));
        return true;
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void set(const char* key, JSON value)
    {
        json_object_set_new(mJson, key, value);
    }

    JSON get(const char* key) const
    {
        return json_object_get(mJson, key);
    }

    JSON mJson = nullptr;
};

template<SectionLock TLock, class TKey, class TValue, class TMap, class TSerializer>
class SerializableTMap
{
    static_assert(std::is_base_of<JSONWrapper<TValue>, TSerializer>::value, "TSerializer is not derived from JSONWrapper<TValue>");
public:
    using TValuePred = std::function<bool(const TValue & value)>;

    virtual ~SerializableTMap() = default;

    bool Add(const TValue & value)
    {
        bool added;
        {
            EXCLUSIVE_ACQUIRE(TLock);
            added = addNoLock(value);
        }
        if(added && DbCallbackBatcher::IsActive())
        {
            DbOperation op {};
            op.opType = DbOperationTypeAdd;

            if(populateDbOperation(op, value))
            {
                DbCallbackBatcher::Add(op);
            }
        }
        return added;
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
        TValue value;
        bool erased;
        {
            EXCLUSIVE_ACQUIRE(TLock);
            auto found = mMap.find(key);
            erased = found != mMap.end();
            if(erased)
            {
                value = std::move(found->second);
                mMap.erase(found);
            }
        }
        if(erased && DbCallbackBatcher::IsActive())
        {
            DbOperation op {};
            op.opType = DbOperationTypeRemove;

            if(populateDbOperation(op, value))
            {
                DbCallbackBatcher::Add(op);
            }
        }
        return erased;
    }

    void DeleteWhere(TValuePred predicate)
    {
        std::vector<TValue> erased;
        {
            EXCLUSIVE_ACQUIRE(TLock);
            for(auto itr = mMap.begin(); itr != mMap.end();)
            {
                if(predicate(itr->second))
                {
                    erased.emplace_back(std::move(itr->second));

                    itr = mMap.erase(itr);
                }
                else
                    ++itr;
            }
        }

        if(DbCallbackBatcher::IsActive())
        {
            for(const TValue & value : erased)
            {
                DbOperation op {};
                op.opType = DbOperationTypeRemove;

                if(populateDbOperation(op, value))
                {
                    DbCallbackBatcher::Add(op);
                }
            }
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

    void Clear(bool terminating)
    {
        TMap empty;
        {
            EXCLUSIVE_ACQUIRE(TLock);
            std::swap(mMap, empty);
        }

        // Do not report termination clear or inactive callback as a DbOperation
        if(terminating || !DbCallbackBatcher::IsActive())
            return;

        for(const auto & kv : empty)
        {
            DbOperation op {};
            op.opType = DbOperationTypeRemove;

            if(populateDbOperation(op, kv.second))
            {
                DbCallbackBatcher::Add(op);
            }
        }
    }

    void CacheSave(JSON root) const
    {
        SHARED_ACQUIRE(TLock);
        auto jsonValues = json_array();
        TSerializer serializer;
        for(const auto & itr : mMap)
        {
            auto jsonValue = json_object();
            serializer.SetJson(jsonValue);
            if(serializer.Save(itr.second))
                json_array_append(jsonValues, jsonValue);
            json_decref(jsonValue);
        }
        if(json_array_size(jsonValues))
            json_object_set(root, jsonKey(), jsonValues);
        json_decref(jsonValues);
    }

    void CacheLoad(JSON root, const char* keyprefix = nullptr)
    {
        auto jsonValues = json_object_get(root, keyprefix ? (keyprefix + String(jsonKey())).c_str() : jsonKey());
        if(!jsonValues)
            return;
        auto active = DbCallbackBatcher::IsActive(true);
        size_t i;
        JSON jsonValue;
        TSerializer deserializer;
        json_array_foreach(jsonValues, i, jsonValue)
        {
            deserializer.SetJson(jsonValue);
            TValue value;
            if(deserializer.Load(value))
            {
                bool added;
                {
                    EXCLUSIVE_ACQUIRE(TLock);
                    added = addNoLock(value);
                }

                if(added && active)
                {
                    DbOperation op {};
                    op.opType = DbOperationTypeAdd;

                    if(populateDbOperation(op, value))
                    {
                        DbCallbackBatcher::Add(op, true);
                    }
                }
            }
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
            *list = TValue(itr.second);
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
    template<typename TMutator>
    bool Modify(TMutator mutator, bool loading = false)
    {
        std::vector<TValue> removed;
        std::vector<TValue> added;
        bool modified;
        {
            EXCLUSIVE_ACQUIRE(TLock);
            modified = mutator(mMap, removed, added);
        }

        if(modified && DbCallbackBatcher::IsActive(loading))
        {
            auto notify = [&]()
            {
                for(const auto & value : removed)
                {
                    DbOperation op {};
                    op.opType = DbOperationTypeRemove;
                    if(populateDbOperation(op, value))
                        DbCallbackBatcher::Add(op, loading);
                }
                for(const auto & value : added)
                {
                    DbOperation op {};
                    op.opType = DbOperationTypeAdd;
                    if(populateDbOperation(op, value))
                        DbCallbackBatcher::Add(op, loading);
                }
            };

            if(removed.size() + added.size() > 1)
            {
                DbCallbackBatcher batcher(loading);
                notify();
            }
            else
            {
                notify();
            }
        }
        return modified;
    }

    virtual const char* jsonKey() const = 0;
    virtual TKey makeKey(const TValue & value) const = 0;
    virtual bool populateDbOperation(DbOperation & op, const TValue & value) const = 0; // Should return whether a database notification is necessary or not

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
            this->Clear(false);
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

struct AddrInfo
{
    duint modhash;
    duint addr;
    bool manual;

    std::string mod() const
    {
        return ModNameFromHash(modhash);
    }
};

struct RangeInfo
{
    duint modhash;
    duint start;
    duint end;
    bool manual;

    std::string mod() const
    {
        return ModNameFromHash(modhash);
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

template<class TValue>
struct RangeInfoSerializer : JSONWrapper<TValue>
{
    static_assert(std::is_base_of<RangeInfo, TValue>::value, "TValue is not derived from RangeInfo");

    bool Save(const TValue & value) override
    {
        this->setString("module", value.mod());
        this->setHex("start", value.start);
        this->setHex("end", value.end);
        this->setBool("manual", value.manual);
        return true;
    }

    bool Load(TValue & value) override
    {
        return loadRangeInfo(value, false);
    }

protected:
    bool loadRangeInfo(TValue & value, bool allowLegacyAddress)
    {
        value.manual = true; // legacy support
        this->getBool("manual", value.manual);
        std::string mod;
        if(!this->getString("module", mod))
            return false;
        value.modhash = ModHashFromName(mod.c_str());

        if(!this->getHex("start", value.start))
        {
            if(!allowLegacyAddress || !this->getHex("address", value.start))
                return false;
            value.end = value.start;
        }
        else if(!this->getHex("end", value.end))
        {
            return false;
        }
        return value.end >= value.start;
    }
};

template<SectionLock TLock, class TValue, class TSerializer>
struct RangeInfoMap : SerializableModuleRangeMap<TLock, TValue, TSerializer>
{
    static_assert(std::is_base_of<RangeInfo, TValue>::value, "TValue is not derived from RangeInfo");
    static_assert(std::is_base_of<RangeInfoSerializer<TValue>, TSerializer>::value, "TSerializer is not derived from RangeInfoSerializer");

    void AdjustValue(TValue & value) const override
    {
        auto base = ModBaseFromName(value.mod().c_str());
        value.start += base;
        value.end += base;
    }

    bool PrepareValue(TValue & value, duint start, duint end, bool manual)
    {
        if(start > end || !MemIsValidReadPtr(start))
            return false;
        auto base = ModBaseFromAddr(start);
        if(base != ModBaseFromAddr(end))
            return false;
        value.modhash = ModHashFromAddr(base);
        value.start = start - base;
        value.end = end - base;
        value.manual = manual;
        return true;
    }

protected:
    ModuleRange makeKey(const TValue & value) const override
    {
        return ModuleRange(value.modhash, Range(value.start, value.end));
    }
};

template<SectionLock TLock, class TValue, class TSerializer>
struct SplitRangeInfoMap : RangeInfoMap<TLock, TValue, TSerializer>
{
    // Replace an inclusive interval, preserving the non-overlapping fragments
    // on either side of every interval it intersects.
    bool ReplaceRange(const TValue & replacement, bool loading = false)
    {
        return this->Modify([&](auto & values, std::vector<TValue> & removed, std::vector<TValue> & added)
        {
            for(auto itr = values.begin(); itr != values.end();)
            {
                const auto & value = itr->second;
                if(value.modhash != replacement.modhash || value.end < replacement.start || value.start > replacement.end)
                {
                    ++itr;
                    continue;
                }

                auto old = value;
                itr = values.erase(itr);
                removed.push_back(old);

                if(old.start < replacement.start)
                {
                    auto left = old;
                    left.end = replacement.start - 1;
                    added.push_back(left);
                }
                if(old.end > replacement.end)
                {
                    auto right = old;
                    right.start = replacement.end + 1;
                    added.push_back(right);
                }
            }

            for(const auto & value : added)
                values.emplace(ModuleRange(value.modhash, Range(value.start, value.end)), value);
            values.emplace(ModuleRange(replacement.modhash, Range(replacement.start, replacement.end)), replacement);
            added.push_back(replacement);
            return true;
        }, loading);
    }

    template<typename TPredicate>
    bool DeleteRangeWhere(duint start, duint end, TPredicate predicate, bool loading = false)
    {
        if(start > end)
            return false;

        const bool all = start == 0 && end == ~duint(0);
        duint modhash = 0;
        if(!all)
        {
            auto base = ModBaseFromAddr(start);
            if(base != ModBaseFromAddr(end))
                return false;
            modhash = ModHashFromAddr(base);
            start -= base;
            end -= base;
        }

        return this->Modify([&](auto & values, std::vector<TValue> & removed, std::vector<TValue> & added)
        {
            for(auto itr = values.begin(); itr != values.end();)
            {
                const auto & value = itr->second;
                if((!all && (value.modhash != modhash || value.end < start || value.start > end)) || !predicate(value))
                {
                    ++itr;
                    continue;
                }

                auto old = value;
                itr = values.erase(itr);
                removed.push_back(old);

                if(!all && old.start < start)
                {
                    auto left = old;
                    left.end = start - 1;
                    added.push_back(left);
                }
                if(!all && old.end > end)
                {
                    auto right = old;
                    right.start = end + 1;
                    added.push_back(right);
                }
            }

            for(const auto & value : added)
                values.emplace(ModuleRange(value.modhash, Range(value.start, value.end)), value);
            return !removed.empty();
        }, loading);
    }
};

#endif // _SERIALIZABLEMAP_H
