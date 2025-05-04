#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace Types
{
    enum Primitive
    {
        Void,
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
        Varargs,
    };

    struct Type
    {
        std::string owner; //Type owner
        std::string name; //Type identifier.
        Primitive primitive = Void; //Primitive type.
        size_t size = 0; //Size in bytes.
    };

    struct QualifiedType
    {
        std::string kind; // struct/class/union/enum
        std::string name; // base name of the type
        bool isConst = false; // whether the base type is const

        QualifiedType() = default;
        explicit QualifiedType(const std::string& name)
            : name(name)
        {
        }

        struct Ptr
        {
            bool isConst = false;
        };

        std::vector<Ptr> pointers; // arbitrary amount of pointers

        std::string pretty() const
        {
            std::string r;
            if(isConst)
                r += "const ";
            r += name;
            for(const auto& ptr : pointers)
            {
                r += '*';
                if(ptr.isConst)
                    r += " const";
            }
            return r;
        }

        std::string noconst() const
        {
            auto r = name;
            for(size_t i = 0; i < pointers.size(); i++)
                r += '*';
            return r;
        }

        bool empty() const
        {
            return name.empty();
        }

        bool isPointer() const
        {
            return !pointers.empty();
        }
    };

    struct Member
    {
        std::string name; //Member identifier
        QualifiedType type; // Qualified Type
        size_t arrsize = 0; //Number of elements if Member is an array (unused for function arguments)
        int offset = -1; //Member offset (only stored for reference)
    };

    enum CallingConvention
    {
        DefaultDecl,
        Cdecl,
        Stdcall,
        Thiscall,
        Delphi
    };

    struct Function
    {
        std::string owner; //Function owner
        std::string name; //Function identifier
        QualifiedType rettype; // Function return type
        CallingConvention callconv = DefaultDecl; //Function calling convention
        bool noreturn = false; //Function does not return (ExitProcess, _exit)
        bool typeonly = false; //Function is only used as a type (the name is based on where it's used)
        std::vector<Member> args; //Function arguments
    };

    struct StructUnion
    {
        std::string owner; //StructUnion owner
        std::string name; //StructUnion identifier
        std::vector<Member> members; //StructUnion members
        std::vector<Function> vtable;
        bool isunion = false; //Is this a union?
        size_t size = 0;
    };

    struct EnumValue
    {
        std::string name;
        uint64_t value = 0;
    };

    struct Enum
    {
        std::string owner; // Enum owner
        std::string name; // Enum name
        std::vector<EnumValue> values; // Enum values
        Primitive type; // Enum value type
    };

    struct Model
    {
        std::vector<Member> types;
        std::vector<std::pair<Enum, std::string>> enums;
        std::vector<StructUnion> structUnions;
        std::vector<Function> functions;
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
            size_t size = 0;
        };

        explicit TypeManager(size_t pointerSize);
        std::string PrimitiveName(Primitive primitive);
        bool AddType(const std::string & owner, const std::string & type, const std::string & name);
        bool AddEnum(const std::string& owner, const std::string& name, const std::string & type);
        bool AddStruct(const std::string & owner, const std::string & name);
        bool AddUnion(const std::string & owner, const std::string & name);
        bool AddMember(const std::string & parent, const QualifiedType & type, const std::string & name, size_t arrsize = 0, int offset = -1);
        bool AppendMember(const QualifiedType & type, const std::string & name, size_t arrsize = 0, int offset = -1);
        bool AddFunction(const std::string & owner, const std::string & name, const QualifiedType& rettype, CallingConvention callconv = Cdecl, bool noreturn = false, bool typeonly = false);
        bool AddEnumerator(const std::string& enumType, const std::string& name, uint64_t value);
        bool AddArg(const std::string & function, const QualifiedType & type, const std::string & name);
        bool AppendArg(const QualifiedType& type, const std::string & name);
        size_t Sizeof(const QualifiedType& type) const;
        size_t Sizeof(const std::string& type) const;
        bool Visit(const std::string & type, const std::string & name, Visitor & visitor) const;
        void Clear(const std::string & owner = "");
        bool RemoveType(const std::string & type);
        void Enumerate(std::vector<Summary> & typeList) const;
        std::string StructUnionPtrType(const std::string & pointto) const;

        bool ParseTypes(const std::string& code, const std::string& owner, std::vector<std::string>& errors);
        bool GenerateStubs() const;

    private:
        std::unordered_map<Primitive, size_t> primitivesizes;
        std::unordered_map<Primitive, std::string> primitivenames;
        std::unordered_map<std::string, Type> types;
        std::unordered_map<std::string, Types::Enum> enums;
        std::unordered_map<std::string, StructUnion> structs;
        std::unordered_map<std::string, Function> functions;
        std::string lastenum;
        std::string laststruct;
        std::string lastfunction;

        bool isDefined(const QualifiedType& type) const;
        bool isDefined(const std::string & id) const;
        bool addStructUnion(const StructUnion & s);
        bool addEnum(const Enum& e);
        bool addType(const std::string & owner, Primitive primitive, const std::string & name);
        bool addType(const Type & t);
        bool visitMember(const Member & root, Visitor & visitor) const;
    };

    bool ParseModel(const std::string& code, const std::string& owner, std::vector<std::string>& errors, Model& model);
};

