#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace Types
{
    enum Primitive
    {
        Void,   // struct/union
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
        PtrWString //wchar_t* (null-terminated)
    };

    struct Type
    {
        std::string owner; //Type owner
        std::string name; //Type identifier.
        std::string pointto; //Type identifier of *Type
        Primitive primitive; //Primitive type.  Void is Struct typedef
        int size = 0; //Size in bytes.
    };

    struct Member
    {
        std::string name; //Member identifier
        std::string type; //Type.name
        int arrsize = 0; //Number of elements if Member is an array
        int offset = -1; //Member offset (only stored for reference)
    };

    struct StructUnion
    {
        std::string owner; //StructUnion owner
        std::string name; //StructUnion identifier
        std::vector<Member> members; //StructUnion members
        bool isunion = false; //Is this a union?
        int size = 0;
    };

    enum CallingConvention
    {
        Cdecl,
        Stdcall,
        Thiscall,
        Delphi
    };

    struct Function
    {
        std::string owner; //Function owner
        std::string name; //Function identifier
        std::string rettype; //Function return type
        CallingConvention callconv = Cdecl; //Function calling convention
        bool noreturn = false; //Function does not return (ExitProcess, _exit)
        std::vector<Member> args; //Function arguments
    };

    struct TypeManager
    {
        struct Visitor
        {
            virtual ~Visitor() { }
            virtual bool visitType(const Member & member, const Type & type) = 0;
            virtual bool visitStructUnion(const Member & member, const StructUnion & type) = 0;
            virtual bool visitArray(const Member & member) = 0;
            virtual bool visitPtr(const Member & member, const Type & type) = 0;
            virtual bool visitBack(const Member & member) = 0;
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
        bool AddStruct(const std::string & owner, const std::string & name);
        bool AddUnion(const std::string & owner, const std::string & name);
        bool AddMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize = 0, int offset = -1);
        bool AppendMember(const std::string & type, const std::string & name, int arrsize = 0, int offset = -1);
        bool AddFunction(const std::string & owner, const std::string & name, const std::string & rettype, CallingConvention callconv = Cdecl, bool noreturn = false);
        bool AddArg(const std::string & function, const std::string & type, const std::string & name);
        bool AppendArg(const std::string & type, const std::string & name);
        int Sizeof(const std::string & type) const;
        bool Visit(const std::string & type, const std::string & name, Visitor & visitor) const;
        void Clear(const std::string & owner = "");
        bool RemoveType(const std::string & type);
        void Enum(std::vector<Summary> & typeList) const;
        std::string StructUnionPtrType(const std::string & pointto) const;

    private:
        std::unordered_map<Primitive, int> primitivesizes;
        std::unordered_map<std::string, Type> types;
        std::unordered_map<std::string, StructUnion> structs;
        std::unordered_map<std::string, Function> functions;
        std::string laststruct;
        std::string lastfunction;

        bool isDefined(const std::string & id) const;
        bool validPtr(const std::string & id);
        bool addStructUnion(const StructUnion & s);
        bool addType(const std::string & owner, Primitive primitive, const std::string & name, const std::string & pointto = "");
        bool addType(const Type & t);
        bool visitMember(const Member & root, Visitor & visitor) const;
    };

    struct Model
    {
        std::vector<Member> types;
        std::vector<StructUnion> structUnions;
        std::vector<Function> functions;
    };
};

bool AddType(const std::string & owner, const std::string & type, const std::string & name);
bool AddStruct(const std::string & owner, const std::string & name);
bool AddUnion(const std::string & owner, const std::string & name);
bool AddMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize = 0, int offset = -1);
bool AppendMember(const std::string & type, const std::string & name, int arrsize = 0, int offset = -1);
bool AddFunction(const std::string & owner, const std::string & name, const std::string & rettype, Types::CallingConvention callconv = Types::Cdecl, bool noreturn = false);
bool AddArg(const std::string & function, const std::string & type, const std::string & name);
bool AppendArg(const std::string & type, const std::string & name);
int SizeofType(const std::string & type);
bool VisitType(const std::string & type, const std::string & name, Types::TypeManager::Visitor & visitor);
void ClearTypes(const std::string & owner = "");
bool RemoveType(const std::string & type);
void EnumTypes(std::vector<Types::TypeManager::Summary> & typeList);
bool LoadTypesJson(const std::string & json, const std::string & owner);
bool LoadTypesFile(const std::string & path, const std::string & owner);
bool ParseTypes(const std::string & parse, const std::string & owner);
std::string StructUnionPtrType(const std::string & pointto);