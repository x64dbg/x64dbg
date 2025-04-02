#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Types
{
    enum Primitive
    {
        Alias, // Typedef/struct/union
        Int8,
        Uint8,
        Int16,
        Uint16,
        Int32,
        Uint32,
        Int64,
        Uint64,
        Dsint,
        Duint,
        Float,
        Double,
        Pointer,
        PtrString, //char* (null-terminated)
        PtrWString, //wchar_t* (null-terminated)
        Void,
    };

    struct TypeBase
    {
        std::string owner; //Type owner (header file name).
        std::string name; //Type identifier.

        uint32_t typeId = 0; // id from typeIdMap
    };

    struct Typedef : TypeBase
    {
        std::string pointto; //Type identifier of *Type
        Primitive primitive; //Primitive type.  Void is Struct typedef
        int sizeBits = 0; //Size in bits.
    };

    struct Member
    {
        std::string assignedType;
        std::string name; //Member identifier
        std::string type; //Type.name

        int arrsize = 0; //Number of elements if Member is an array
        int bitSize = -1; //Bitfield size
        int offsetBits = -1; //Member offset (only stored for reference)

        bool bitfield = false;
    };

    struct StructUnion : TypeBase
    {
        std::vector<Member> members; //StructUnion members
        bool isUnion = false; //Is this a union?
        int sizeBits = -1;
    };

    enum CallingConvention
    {
        Cdecl,
        Stdcall,
        Thiscall,
        Delphi
    };

    struct Function : TypeBase
    {
        std::string rettype; //Function return type
        CallingConvention callconv = Cdecl; //Function calling convention
        bool noreturn = false; //Function does not return (ExitProcess, _exit)
        std::vector<Member> args; //Function arguments
    };

    struct Enum : TypeBase
    {
        std::vector<std::pair<uint64_t, std::string>> members;
        uint8_t sizeFUCK;
        bool isFlags;
    };

    struct TypeManager
    {
        struct Visitor
        {
            virtual ~Visitor()
            {
            }

            virtual bool visitType(const Member & member, const Typedef & type) = 0;
            virtual bool visitStructUnion(const Member & member, const StructUnion & type) = 0;
            virtual bool visitArray(const Member & member) = 0;
            virtual bool visitPtr(const Member & member, const Typedef & type) = 0;
            virtual bool visitBack(const Member & member) = 0;
            virtual bool visitEnum(const Member & member, const Enum & num) = 0;
        };

        struct Summary
        {
            std::string kind;
            std::string name;
            std::string owner;
            int size = 0;
        };

        explicit TypeManager();
        bool AddType(const std::string & owner, const std::string & type, const std::string & name);
        bool AddStruct(const std::string & owner, const std::string & name, int constantSize = -1);
        bool AddUnion(const std::string & owner, const std::string & name, int constantSize = -1);
        bool AddEnum(const std::string & owner, const std::string & name, bool isFlags, uint8_t size);
        bool AddEnumMember(const std::string & parent, const std::string & name, uint64_t value);
        bool AddStructMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize, int bitOffset, int bitSize, bool isBitfield);
        bool AppendStructMember(const std::string & type, const std::string & name, int arrsize = 0, int offset = -1);
        bool AppendStructPadding(const std::string & type, int targetOffset);
        bool AddFunction(const std::string & owner, const std::string & name, CallingConvention callconv = Cdecl,
                         bool noreturn = false);
        bool AddFunctionReturn(const std::string & name, const std::string & rettype);
        bool AddArg(const std::string & function, const std::string & type, const std::string & name);
        bool AppendArg(const std::string & type, const std::string & name);
        int Sizeof(const std::string & type,
                   std::string* underlyingType = nullptr);
        Types::TypeBase* LookupTypeById(uint32_t typeId);
        bool Visit(const std::string & type, const std::string & name, Visitor & visitor) const;
        void Clear(const std::string & owner = "");
        bool RemoveType(const std::string & type);
        void Enum(std::vector<Summary> & typeList) const;
        std::string StructUnionPtrType(const std::string & pointto) const;

    private:
        uint32_t currentTypeId = 100;
        std::unordered_map<uint32_t, TypeBase*> typeIdMap;

        std::unordered_map<Primitive, int> primitivesizes;
        std::unordered_map<std::string, Typedef> types;
        std::unordered_map<std::string, struct Enum> enums;
        std::unordered_map<std::string, StructUnion> structs;
        std::unordered_map<std::string, Function> functions;
        std::string laststruct;
        std::string lastfunction;

        bool isDefined(const std::string & id) const;
        bool validPtr(const std::string & id);
        bool addStructUnion(const StructUnion & s);
        bool addType(const std::string & owner, Primitive primitive, const std::string & name, const std::string & pointto = "");
        bool addType(const Typedef & t);
        bool visitMember(const Member & root, Visitor & visitor) const;

        template <typename K, typename V>
        bool removeType(std::unordered_map<K, V> & map, const std::string & type)
        {
            auto found = map.find(type);
            if(found == map.end())
                return false;
            if(found->second.owner.empty())
                return false;
            typeIdMap.erase(found->second.typeId);
            map.erase(found);
            return true;
        }

        template <typename K, typename V>
        void filterOwnerMap(std::unordered_map<K, V> & map, const std::string & owner)
        {
            for(auto i = map.begin(); i != map.end();)
            {
                auto j = i++;
                if(j->second.owner.empty())
                    continue;
                if(owner.empty() || j->second.owner == owner)
                {
                    typeIdMap.erase(j->second.typeId);
                    map.erase(j);
                }
            }
        }
    };

    struct Model
    {
        std::vector<Member> types;
        std::vector<StructUnion> structUnions;
        std::vector<Function> functions;
        std::vector<Enum> enums;
    };
};

bool AddType(const std::string & owner, const std::string & type, const std::string & name);
bool AddStruct(const std::string & owner, const std::string & name);
bool AddUnion(const std::string & owner, const std::string & name);
bool AddMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize = 0, int offset = -1);
bool AppendMember(const std::string & type, const std::string & name, int arrsize = 0, int offset = -1);
bool AddFunction(const std::string & owner, const std::string & name, const std::string & rettype, Types::CallingConvention callconv = Types::Cdecl,
                 bool noreturn = false);
bool AddArg(const std::string & function, const std::string & type, const std::string & name);
bool AppendArg(const std::string & type, const std::string & name);
Types::TypeBase* LookupTypeById(uint32_t typeId);
int SizeofType(const std::string & type);
bool VisitType(const std::string & type, const std::string & name, Types::TypeManager::Visitor & visitor);
void ClearTypes(const std::string & owner = "");
bool RemoveType(const std::string & type);
void EnumTypes(std::vector<Types::TypeManager::Summary> & typeList);
bool LoadTypesJson(const std::string & json, const std::string & owner);
bool LoadTypesFile(const std::string & path, const std::string & owner);
bool ParseTypes(const std::string & parse, const std::string & owner);
std::string StructUnionPtrType(const std::string & pointto);
