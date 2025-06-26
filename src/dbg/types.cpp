#include "types.h"
#include "stringutils.h"
#include "threading.h"
#include "filehelper.h"
#include "console.h"
#include "jansson/jansson_x64dbg.h"
#include <algorithm>

using namespace Types;

static TypeManager typeManager;

TypeManager::TypeManager()
{
    auto p = [this](const std::string & n, Primitive p, int size)
    {
        primitiveSizes[p] = size;
        auto splits = StringUtils::Split(n, ',');
        for(const auto & split : splits)
            addType("", p, split);
    };
    p("int8_t,int8,char,byte,bool,signed char", Int8, sizeof(char));
    p("uint8_t,uint8,uchar,unsigned char,ubyte", Uint8, sizeof(unsigned char));
    p("int16_t,int16,wchar_t,char16_t,short", Int16, sizeof(short));
    p("uint16_t,uint16,ushort,unsigned short", Uint16, sizeof(unsigned short));
    p("int32_t,int32,int,long", Int32, sizeof(int));
    p("uint32_t,uint32,unsigned int,unsigned long", Uint32, sizeof(unsigned int));
    p("int64_t,int64,long long", Int64, sizeof(long long));
    p("uint64_t,uint64,unsigned long long", Uint64, sizeof(unsigned long long));
    p("dsint", Dsint, sizeof(void*));
    p("duint,size_t", Duint, sizeof(void*));
    p("float", Float, sizeof(float));
    p("double,long double", Double, sizeof(double));
    p("ptr,void*", Pointer, sizeof(void*));
    p("char*,const char*", PtrString, sizeof(char*));
    p("wchar_t*,const wchar_t*", PtrWString, sizeof(wchar_t*));
}

bool TypeManager::AddType(const std::string & owner, const std::string & type, const std::string & name)
{
    if(owner.empty())
        return false;
    validPtr(type);

    auto foundType = types.find(type);
    if(foundType != types.end())
    {
        if(foundType->second.primitive == Alias)
            return addType(owner, Alias, name, type);
        if(foundType->second.primitive == Pointer)
            return addType(owner, Alias, name, type);

        return addType(owner, foundType->second.primitive, name);
    }

    auto foundE = enums.find(type);
    if(foundE != enums.end())
        return addType(owner, Alias, name, type);

    auto foundS = structs.find(type);
    if(foundS != structs.end())
        return addType(owner, Alias, name, type);

    auto foundF = functions.find(type);
    if(foundF != functions.end())
        return addType(owner, Alias, name, type);

    if(type == "void")
        return addType(owner, Void, name);

    return false;
}

bool TypeManager::AddStruct(const std::string & owner, const std::string & name, int constantSize)
{
    StructUnion s;
    s.name = name;
    s.owner = owner;
    s.sizeBits = constantSize;

    return addStructUnion(s);
}

bool TypeManager::AddUnion(const std::string & owner, const std::string & name, int constantSize)
{
    StructUnion u;
    u.owner = owner;
    u.name = name;
    u.isUnion = true;
    u.sizeBits = constantSize;

    return addStructUnion(u);
}

bool TypeManager::AddEnum(const std::string & owner, const std::string & name, bool isFlags, uint8_t size)
{
    struct Enum e;
    e.owner = owner;
    e.name = name;
    e.members = { };
    e.isFlags = isFlags;
    e.sizeBits = size;

    if(enums.find(name) != enums.end())
        return false;

    auto & type = enums.insert({ e.name, e }).first->second;
    typeIdMap[currentTypeId] = &type;
    type.typeId = currentTypeId++;

    return true;
}

bool TypeManager::AddEnumMember(const std::string & parent, const std::string & name, uint64_t value)
{
    auto found = enums.find(parent);
    if(found == enums.end())
        return false;

    found->second.members.emplace_back(value, name);
    return true;
}

bool TypeManager::AddStructMember(const std::string & parent, const std::string & type, const std::string & name, int arraySize, int bitOffset, int sizeBits, bool isBitfield)
{
    if(!isDefined(type) && !validPtr(type))
        return false;
    auto found = structs.find(parent);
    if(arraySize < 0 || found == structs.end() || !isDefined(type) || name.empty() || type.empty() || type == parent)
        return false;

    // cannot have bit field array
    if(isBitfield && arraySize != 0)
        return false;

    // cannot be pointer and bitfield
    int typeSize;
    if(type.back() == '*')
    {
        // this is a pointer type
        auto expectedSize = primitiveSizes[Pointer] * 8;
        if(arraySize != 0)
            expectedSize *= arraySize;

        if(sizeBits != -1 && sizeBits != expectedSize)
            return false;

        typeSize = expectedSize;
    }
    else
    {
        if(arraySize != 0)
            typeSize = Sizeof(type) * arraySize;
        else
            typeSize = Sizeof(type);
    }

    auto & s = found->second;
    for(const auto & member : s.members)
        if(member.name == name)
            return false;

    // cannot have a bitfield greater than typeSize
    if(isBitfield)
    {
        if(sizeBits > typeSize)
            return false;

        typeSize = sizeBits;
    }

    if(typeSize == -1)
        __debugbreak();

    Member m;
    m.name = name;
    m.arraySize = arraySize;
    m.type = type;
    m.offsetBits = bitOffset;
    m.sizeBits = typeSize;
    m.isBitfield = isBitfield;

    if(bitOffset >= 0 && !s.isUnion)  //user-defined offset
    {
        if(bitOffset < s.sizeBits)
            return false;

        if(bitOffset > s.sizeBits)
            AppendStructPadding(parent, bitOffset);
    }

    if(s.isUnion)
    {
        if(typeSize > s.sizeBits)
            s.sizeBits = typeSize;
    }
    else
    {
        s.sizeBits += m.sizeBits;
    }

    s.members.push_back(m);
    return true;
}

bool TypeManager::AppendStructMember(const std::string & type, const std::string & name, int arraySize, int offset)
{
    return AddStructMember(lastStruct, type, name, arraySize, offset, -1, false);
}

bool TypeManager::AppendStructPadding(const std::string & parent, int targetOffset)
{
    auto found = structs.find(parent);
    if(found == structs.end())
        return false;

    auto & s = found->second;
    if(s.sizeBits >= targetOffset || s.isUnion)
        return false;

    auto bitPadding = targetOffset - s.sizeBits;
    if(bitPadding % 8 != 0)
    {
        const auto remBits = bitPadding % 8;

        Member pad;
        pad.type = "long long";
        pad.sizeBits = remBits;
        pad.isBitfield = true;

        char padName[32] = { };
        sprintf_s(padName, "padding%db", remBits);

        pad.name = padName;
        s.members.push_back(pad);

        // more to add
        bitPadding -= remBits;
    }

    if(bitPadding)
    {
        Member pad;
        pad.type = "char";
        pad.sizeBits = bitPadding;
        pad.arraySize = bitPadding / 8;

        // can just add byte padding
        char padName[32] = { };
        sprintf_s(padName, "padding%d", bitPadding / 8);

        pad.name = padName;
        s.members.push_back(pad);
    }

    s.sizeBits = targetOffset;
    return true;
}

bool TypeManager::AddFunction(const std::string & owner, const std::string & name, CallingConvention callconv,
                              bool noreturn)
{
    auto found = functions.find(name);
    if(found != functions.end() || name.empty() || owner.empty())
        return false;
    lastFunction = name;
    Function f;
    f.owner = owner;
    f.name = name;
    f.returnType = "";

    // return type cannot be set yet

    f.callconv = callconv;
    f.noreturn = noreturn;
    typeIdMap[currentTypeId++] = &functions.insert({ f.name, f }).first->second;
    return true;
}

bool TypeManager::AddFunctionReturn(const std::string & name, const std::string & returnType)
{
    auto found = functions.find(name);
    if(found == functions.end() || name.empty())
        return false;

    Function & f = found->second;
    if(returnType != "void" && !isDefined(returnType) && !validPtr(returnType))
        return false;
    f.returnType = returnType;
    return true;
}

bool TypeManager::AddArg(const std::string & function, const std::string & type, const std::string & name)
{
    if(!isDefined(type) && !validPtr(type))
        return false;
    auto found = functions.find(function);
    if(found == functions.end() || function.empty() || name.empty() || !isDefined(type))
        return false;
    lastFunction = function;
    Member arg;
    arg.name = name;
    arg.type = type;
    found->second.args.push_back(arg);
    return true;
}

bool TypeManager::AppendArg(const std::string & type, const std::string & name)
{
    return AddArg(lastFunction, type, name);
}

int TypeManager::Sizeof(const std::string & type, std::string* underlyingType)
{
    if(!types.empty() && type.back() == '*')
    {
        if(underlyingType != nullptr)
            *underlyingType = type;
        return primitiveSizes[Pointer] * 8;
    }

    auto foundT = types.find(type);
    if(foundT != types.end())
    {
        if(!foundT->second.alias.empty())
            return Sizeof(foundT->second.alias, underlyingType);

        if(underlyingType != nullptr)
            *underlyingType = foundT->second.name;
        return foundT->second.sizeBits;
    }

    auto foundE = enums.find(type);
    if(foundE != enums.end())
    {
        if(underlyingType != nullptr)
            *underlyingType = foundE->second.name;
        return foundE->second.sizeBits;
    }

    auto foundS = structs.find(type);
    if(foundS != structs.end())
    {
        if(underlyingType != nullptr)
            *underlyingType = foundS->second.name;
        return foundS->second.sizeBits;
    }

    auto foundF = functions.find(type);
    if(foundF != functions.end())
    {
        if(underlyingType != nullptr)
            *underlyingType = foundF->second.name;
        return primitiveSizes[Pointer] * 8;
    }

    return 0;
}

TypeBase* TypeManager::LookupTypeById(const uint32_t typeId)
{
    const auto type = typeIdMap.find(typeId);
    if(type == typeIdMap.end())
        return nullptr;

    return type->second;
}

TypeBase* TypeManager::LookupTypeByName(const std::string & typeName)
{
    auto foundT = types.find(typeName);
    if(foundT != types.end())
        return &foundT->second;

    auto foundE = enums.find(typeName);
    if(foundE != enums.end())
        return &foundE->second;

    auto foundS = structs.find(typeName);
    if(foundS != structs.end())
        return &foundS->second;

    auto foundF = functions.find(typeName);
    if(foundF != functions.end())
        return &foundF->second;

    return nullptr;
}

bool TypeManager::Visit(const std::string & type, const std::string & name, Visitor & visitor) const
{
    Member m;
    m.name = name;
    m.type = type;

    return visitMember(m, visitor, m.type);
}

void TypeManager::Clear(const std::string & owner)
{
    lastStruct.clear();
    lastFunction.clear();
    filterOwnerMap(types, owner);
    filterOwnerMap(structs, owner);
    filterOwnerMap(functions, owner);
    filterOwnerMap(enums, owner);
}

bool TypeManager::RemoveType(const std::string & type)
{
    return removeType(types, type) || removeType(structs, type) || removeType(functions, type) || removeType(enums, type);
}

static std::string getKind(const StructUnion & su)
{
    return su.isUnion ? "union" : "struct";
}

static std::string getKind(const Typedef & t)
{
    return "typedef";
}

static std::string getKind(const Function & f)
{
    return "function";
}

template <typename K, typename V>
static void enumType(const std::unordered_map<K, V> & map, std::vector<TypeManager::Summary> & types)
{
    for(auto i = map.begin(); i != map.end(); ++i)
    {
        TypeManager::Summary s;
        s.kind = getKind(i->second);
        s.name = i->second.name;
        s.owner = i->second.owner;
        s.size = SizeofType(s.name);
        types.push_back(s);
    }
}

void TypeManager::Enum(std::vector<Summary> & typeList) const
{
    typeList.clear();
    enumType(types, typeList);
    enumType(structs, typeList);
    enumType(functions, typeList);
    //nasty hacks to sort in a nice way
    std::sort(typeList.begin(), typeList.end(), [](const Summary & a, const Summary & b)
    {
        auto kindInt = [](const std::string & kind)
        {
            if(kind == "typedef")
                return 0;
            if(kind == "struct")
                return 1;
            if(kind == "union")
                return 2;
            if(kind == "function")
                return 3;
            __debugbreak();
            return 4;
        };
        if(a.owner < b.owner)
            return true;
        else if(a.owner > b.owner)
            return false;
        auto ka = kindInt(a.kind), kb = kindInt(b.kind);
        if(ka < kb)
            return true;
        else if(ka > kb)
            return false;
        if(a.name < b.name)
            return true;
        else if(a.name > b.name)
            return false;
        return a.size < b.size;
    });
}

std::string Types::TypeManager::StructUnionPtrType(const std::string & alias) const
{
    auto itr = structs.find(alias);
    if(itr == structs.end())
        return "";
    return getKind(itr->second);
}

template <typename K, typename V>
static bool mapContains(const std::unordered_map<K, V> & map, const K & k)
{
    return map.find(k) != map.end();
}

bool TypeManager::isDefined(const std::string & id) const
{
    return mapContains(types, id) || mapContains(structs, id) || mapContains(enums, id) || mapContains(functions, id);
}

bool TypeManager::validPtr(const std::string & id)
{
    if(id[id.length() - 1] == '*')
    {
        auto type = id.substr(0, id.length() - 1);
        if(!isDefined(type) && !validPtr(type))
            return false;

        std::string owner("ptr");
        auto foundT = types.find(type);
        if(foundT != types.end())
            owner = foundT->second.owner;
        auto foundS = structs.find(type);
        if(foundS != structs.end())
            owner = foundS->second.owner;
        auto foundE = enums.find(type);
        if(foundE != enums.end())
            owner = foundE->second.owner;
        auto foundP = functions.find(type);
        if(foundP != functions.end())
            owner = foundP->second.owner;

        return addType(owner, Pointer, id, type);
    }

    return false;
}

bool TypeManager::addStructUnion(const StructUnion & s)
{
    lastStruct = s.name;
    if(s.owner.empty() || s.name.empty() || isDefined(s.name))
        return false;

    auto & type = structs.insert({ s.name, s }).first->second;
    typeIdMap[currentTypeId] = &type;
    type.typeId = currentTypeId++;

    return true;
}

bool TypeManager::addType(const Typedef & t)
{
    if(t.name.empty() || isDefined(t.name))
        return false;

    auto & type = types.insert({ t.name, t }).first->second;
    typeIdMap[currentTypeId] = &type;
    type.typeId = currentTypeId++;

    return true;
}

bool TypeManager::addType(const std::string & owner, Primitive primitive, const std::string & name, const std::string & alias)
{
    auto foundT = types.find(name);
    if(foundT != types.end())
    {
        if(alias.empty())
        {
            if(primitiveSizes[primitive] == primitiveSizes[foundT->second.primitive])
                return true;
        }
        else
        {
            std::string underlyingType;
            Sizeof(alias, &underlyingType);

            if(underlyingType == alias)
                return true;
        }
    }

    if(name.empty() || isDefined(name))
        return false;
    Typedef t;
    t.owner = owner;
    t.name = name;
    t.primitive = primitive;
    t.sizeBits = primitiveSizes[primitive] * 8;
    t.alias = alias;
    return addType(t);
}

bool TypeManager::visitMember(const Member & root, Visitor & visitor, const std::string & prettyType) const
{
    auto foundT = types.find(root.type);
    if(foundT != types.end())
    {
        const auto & t = foundT->second;
        if(t.primitive == Alias) // check if struct type
        {
            Member member = root;
            member.type = t.alias;

            return visitMember(member, visitor, prettyType);
        }

        if(!t.alias.empty())
        {
            if(!isDefined(t.alias))
                return false;
            if(visitor.visitPtr(root, t, prettyType)) //allow the visitor to bail out
            {
                if(!Visit(t.alias, "*" + root.name, visitor))
                    return false;
                return visitor.visitBack(root);
            }
            return true;
        }

        return visitor.visitType(root, t, prettyType);
    }

    auto foundS = structs.find(root.type);
    if(foundS != structs.end())
    {
        const auto & s = foundS->second;
        if(!visitor.visitStructUnion(root, s, prettyType))
            return false;

        for(const auto & child : s.members)
        {
            if(child.arraySize > 0)
            {
                if(!visitor.visitArray(child, child.type))
                    return false;

                Member am = child;
                am.sizeBits = child.sizeBits / child.arraySize;
                am.arraySize = -1;

                for(auto i = 0; i < child.arraySize; i++)
                    if(!visitMember(am, visitor, am.type))
                        return false;

                if(!visitor.visitBack(child))
                    return false;
            }
            else if(!visitMember(child, visitor, child.type))
                return false;
        }
        return visitor.visitBack(root);
    }

    auto foundE = enums.find(root.type);
    if(foundE != enums.end())
    {
        const auto & e = foundE->second;
        return visitor.visitEnum(root, e, prettyType);
    }

    auto foundF = functions.find(root.type);
    if(foundF != functions.end())
    {
        const auto & function = foundF->second;

        std::string functionType = function.noreturn ? "__noreturn " : "";
        functionType += function.returnType;
        functionType += "(";
        for(size_t i = 0; i < function.args.size(); i++)
        {
            if(i > 0)
                functionType += ", ";
            functionType += function.args[i].type;
        }
        functionType += ")*";

        Member fm;
        fm.name = function.name;
        fm.type = functionType;
        fm.offsetBits = root.offsetBits;

        Typedef ft;
        ft.primitive = Pointer;
        ft.sizeBits = primitiveSizes.at(Pointer) * 8;

        return visitor.visitType(fm, ft, fm.type);
    }

    return false;
}

bool AddType(const std::string & owner, const std::string & type, const std::string & name)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddType(owner, type, name);
}

bool AddStruct(const std::string & owner, const std::string & name)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddStruct(owner, name);
}

bool AddUnion(const std::string & owner, const std::string & name)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddUnion(owner, name);
}

bool AddMember(const std::string & parent, const std::string & type, const std::string & name, int arraySize, int offset)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddStructMember(parent, type, name, arraySize, offset, -1, false);
}

bool AppendMember(const std::string & type, const std::string & name, int arraySize, int offset)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AppendStructMember(type, name, arraySize, offset);
}

bool AddFunction(const std::string & owner, const std::string & name, const std::string & returnType, Types::CallingConvention callconv, bool noreturn)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddFunction(owner, name, callconv, noreturn);
}

bool AddArg(const std::string & function, const std::string & type, const std::string & name)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddArg(function, type, name);
}

bool AppendArg(const std::string & type, const std::string & name)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AppendArg(type, name);
}

TypeBase* LookupTypeById(uint32_t typeId)
{
    SHARED_ACQUIRE(LockTypeManager);
    return typeManager.LookupTypeById(typeId);
}

TypeBase* LookupTypeByName(const std::string & name)
{
    SHARED_ACQUIRE(LockTypeManager);
    return typeManager.LookupTypeByName(name);
}

int SizeofType(const std::string & type)
{
    SHARED_ACQUIRE(LockTypeManager);
    return typeManager.Sizeof(type);
}

bool VisitType(const std::string & type, const std::string & name, Types::TypeManager::Visitor & visitor)
{
    SHARED_ACQUIRE(LockTypeManager);
    return typeManager.Visit(type, name, visitor);
}

void ClearTypes(const std::string & owner)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.Clear(owner);
}

bool RemoveType(const std::string & type)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.RemoveType(type);
}

void EnumTypes(std::vector<Types::TypeManager::Summary> & typeList)
{
    SHARED_ACQUIRE(LockTypeManager);
    return typeManager.Enum(typeList);
}

int json_default_int(const JSON object, const char* key, int defaultVal)
{
    auto jint = json_object_get(object, key);
    if(jint && json_is_integer(jint))
        return int(json_integer_value(jint));
    return defaultVal;
}

static void loadTypes(const JSON troot, std::vector<Member> & types)
{
    if(!troot)
        return;
    size_t i;
    JSON vali;
    Member curType;
    types.reserve(json_array_size(troot));
    json_array_foreach(troot, i, vali)
    {
        auto type = json_string_value(json_object_get(vali, "type"));
        auto name = json_string_value(json_object_get(vali, "name"));
        if(!type || !*type || !name || !*name)
            continue;
        curType.type = type;
        curType.name = name;
        types.push_back(curType);
    }
}

static void loadStructUnions(const JSON suroot, std::vector<StructUnion> & structUnions)
{
    if(!suroot)
        return;
    size_t i;
    JSON vali;
    StructUnion curSu;
    json_array_foreach(suroot, i, vali)
    {
        auto suname = json_string_value(json_object_get(vali, "name"));
        if(!suname || !*suname)
            continue;

        curSu.name = suname;
        curSu.members.clear();
        curSu.isUnion = json_boolean_value(json_object_get(vali, "isUnion"));

        auto size = json_default_int(vali, "size", -1);
        if(size == -1)
            size = json_default_int(vali, "sizeBits", 8);
        else
            size *= 8;

        curSu.sizeBits = size;

        auto members = json_object_get(vali, "members");

        size_t j;
        JSON valj;
        Member curMember;
        json_array_foreach(members, j, valj)
        {
            auto type = json_string_value(json_object_get(valj, "type"));
            auto name = json_string_value(json_object_get(valj, "name"));
            if(!type || !*type || !name || !*name)
                continue;

            curMember.type = type;
            curMember.name = name;
            curMember.arraySize = json_default_int(valj, "arrsize", 0);
            // NOTE: Attempt to keep support for the previous JSON format
            size = json_default_int(valj, "size", -1);
            if(size == -1)
                size = json_default_int(valj, "sizeBits", 8);
            else
                size *= 8;
            curMember.sizeBits = size;
            curMember.isBitfield = json_boolean_value(json_object_get(valj, "bitfield"));

            curMember.offsetBits = json_default_int(valj, "offset", -1);
            if(curMember.offsetBits == -1)
                curMember.offsetBits = json_default_int(valj, "bitOffset", -1);
            else
                curMember.offsetBits *= 8;

            curSu.members.push_back(curMember);
        }
        structUnions.push_back(curSu);
    }
}

static void loadFunctions(const JSON froot, std::vector<Function> & functions)
{
    if(!froot)
        return;
    size_t i;
    JSON vali;
    Function curFunction;
    functions.reserve(json_array_size(froot));
    json_array_foreach(froot, i, vali)
    {
        auto returnType = json_string_value(json_object_get(vali, "rettype"));
        auto fname = json_string_value(json_object_get(vali, "name"));
        if(!returnType || !*returnType || !fname || !*fname)
            continue;
        curFunction.returnType = returnType;
        curFunction.name = fname;
        curFunction.args.clear();
        auto callconv = json_string_value(json_object_get(vali, "callconv"));
        curFunction.noreturn = json_boolean_value(json_object_get(vali, "noreturn"));
        if(scmp(callconv, "cdecl"))
            curFunction.callconv = Cdecl;
        else if(scmp(callconv, "stdcall"))
            curFunction.callconv = Stdcall;
        else if(scmp(callconv, "thiscall"))
            curFunction.callconv = Thiscall;
        else if(scmp(callconv, "delphi"))
            curFunction.callconv = Delphi;
        else
            curFunction.callconv = Cdecl;
        auto args = json_object_get(vali, "args");
        size_t j;
        JSON valj;
        Member curArg;
        curFunction.args.reserve(json_array_size(args));
        json_array_foreach(args, j, valj)
        {
            auto type = json_string_value(json_object_get(valj, "type"));
            auto name = json_string_value(json_object_get(valj, "name"));
            if(!type || !*type || !name || !*name)
                continue;
            curArg.type = type;
            curArg.name = name;
            curFunction.args.push_back(curArg);
        }
        functions.push_back(curFunction);
    }
}

static void loadEnums(const JSON suroot, std::vector<struct Enum> & enums)
{
    if(!suroot)
        return;
    size_t i;
    JSON vali;

    Enum curE;
    json_array_foreach(suroot, i, vali)
    {
        auto suname = json_string_value(json_object_get(vali, "name"));
        if(!suname || !*suname)
            continue;
        curE.name = suname;
        curE.members.clear();

        auto size = json_default_int(vali, "size", -1);
        if(size == -1)
            size = json_default_int(vali, "sizeBits", 8);
        else
            size *= 8;

        curE.sizeBits = size;
        curE.isFlags = json_boolean(json_object_get(vali, "isFlags"));

        auto members = json_object_get(vali, "members");
        size_t j;
        JSON valj;

        std::pair<uint64_t, std::string> curMember;
        json_array_foreach(members, j, valj)
        {
            const auto valueObject = json_object_get(valj, "name");
            if(!valueObject)
                continue;

            auto value = json_default_int(valj, "value", 0);
            auto name = json_string_value(json_object_get(valj, "name"));
            if(value == -1 || !name || !*name)
                continue;

            curMember.first = value;
            curMember.second = name;
            curE.members.push_back(curMember);
        }

        enums.push_back(curE);
    }
}

int LoadModel(const std::string & owner, Model & model)
{
    int errorCount = 0;

    //Add all base struct/union types first to avoid errors later
    for(auto & su : model.structUnions)
    {
        auto success = su.isUnion ? typeManager.AddUnion(owner, su.name, 0) : typeManager.AddStruct(owner, su.name, 0);
        if(!success)
        {
            //TODO properly handle errors
            errorCount++;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add %s %s;\n"), su.isUnion ? "union" : "struct", su.name.c_str());
            su.name.clear(); //signal error
        }
    }

    //Add base function types to avoid errors later
    for(auto & function : model.functions)
    {
        auto success = typeManager.AddFunction(owner, function.name, function.callconv, function.noreturn);
        if(!success)
        {
            //TODO properly handle errors
            errorCount++;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add function %s %s()\n"), function.returnType.c_str(), function.name.c_str());
            function.name.clear(); //signal error
        }
    }

    for(auto & num : model.enums)
    {
        if(num.name.empty())  //skip error-signalled functions
            continue;

        auto success = typeManager.AddEnum(owner, num.name, num.isFlags, num.sizeBits);
        if(!success)
        {
            //TODO properly handle errors
            errorCount++;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add enum %s\n"), num.name.c_str());
        }
    }

    //Add simple typedefs
    for(auto & type : model.types)
    {
        auto success = typeManager.AddType(owner, type.type, type.name);
        if(!success)
        {
            //TODO properly handle errors
            errorCount++;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add typedef %s %s;\n"), type.type.c_str(), type.name.c_str());
        }
    }

    //Add struct/union members
    for(auto & su : model.structUnions)
    {
        if(su.name.empty())  //skip error-signalled structs/unions
            continue;

        const auto suggestedSize = su.sizeBits;
        for(auto & member : su.members)
        {
            auto success = typeManager.AddStructMember(su.name, member.type, member.name, member.arraySize, member.offsetBits, member.sizeBits, member.isBitfield);
            if(!success)
            {
                //TODO properly handle errors
                errorCount++;
                dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add member %s %s.%s;\n"), member.type.c_str(), su.name.c_str(), member.name.c_str());
            }
        }

        if(suggestedSize != 0)
            typeManager.AppendStructPadding(su.name, suggestedSize);
    }

    for(auto & num : model.enums)
    {
        if(num.name.empty())  //skip error-signalled functions
            continue;

        for(auto & mem : num.members)
        {
            auto success = typeManager.AddEnumMember(num.name, mem.second, mem.first);
            if(!success)
            {
                //TODO properly handle errors
                errorCount++;
                dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add enum member %s\n"), mem.second.c_str());
            }
        }
    }

    //Add function arguments
    for(auto & function : model.functions)
    {
        if(function.name.empty())  //skip error-signalled functions
            continue;

        bool status = typeManager.AddFunctionReturn(function.name, function.returnType);
        if(!status)
        {
            //TODO properly handle errors
            errorCount++;
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add return type %s.%s;\n"), function.name.c_str(), function.returnType.c_str());
        }

        for(auto & arg : function.args)
        {
            auto success = typeManager.AddArg(function.name, arg.type, arg.name);
            if(!success)
            {
                //TODO properly handle errors
                errorCount++;
                dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add argument %s %s.%s;\n"), arg.type.c_str(), function.name.c_str(), arg.name.c_str());
            }
        }
    }
    return errorCount;
}

bool LoadTypesJson(const std::string & json, const std::string & owner)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    json_error_t err;
    auto root = json_loads(json.c_str(), 0, &err);
    if(root)
    {
        Model model;
        loadTypes(json_object_get(root, "types"), model.types);
        loadTypes(json_object_get(root, ArchValue("types32", "types64")), model.types);
        loadStructUnions(json_object_get(root, "structUnions"), model.structUnions);
        loadFunctions(json_object_get(root, "functions"), model.functions);
        loadFunctions(json_object_get(root, ArchValue("functions32", "functions64")), model.functions);
        loadEnums(json_object_get(root, "enums"), model.enums);
        loadEnums(json_object_get(root, ArchValue("enums32", "enums64")), model.enums);

        LoadModel(owner, model);

        // Free root
        json_decref(root);
    }
    else
        return false;
    return true;
}

bool LoadTypesFile(const std::string & path, const std::string & owner)
{
    std::string json;
    if(!FileHelper::ReadAllText(path, json))
        return false;
    return LoadTypesJson(json, owner);
}

std::string StructUnionPtrType(const std::string & alias)
{
    return typeManager.StructUnionPtrType(alias);
}
