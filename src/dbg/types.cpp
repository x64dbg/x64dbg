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
        primitivesizes[p] = size;
        auto splits = StringUtils::Split(n, ',');
        for(const auto & split : splits)
            addType("", p, split);
    };
    p("int8_t,int8,char,byte,bool,signed char", Int8, sizeof(char));
    p("uint8_t,uint8,uchar,unsigned char,ubyte", Uint8, sizeof(unsigned char));
    p("int16_t,int16,wchar_t,char16_t,short", Int16, sizeof(short));
    p("uint16_t,uint16,ushort,unsigned short", Int16, sizeof(unsigned short));
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
        if(foundType->second.primitive == Typedef)
            return addType(owner, Typedef, name, type);
        if(foundType->second.primitive == Pointer)
            return addType(owner, Typedef, name, type);

        return addType(owner, foundType->second.primitive, name);
    }

    auto found_e = enums.find(type);
    if(found_e != enums.end())
        return addType(owner, Typedef, name, type);

    auto found_s = structs.find(type);
    if(found_s != structs.end())
        return addType(owner, Typedef, name, type);

    auto found_f = functions.find(type);
    if(found_f != functions.end())
        return addType(owner, Typedef, name, type);

    if(type == "void")
        return addType(owner, Void, name);

    return false;
}

bool TypeManager::AddStruct(const std::string & owner, const std::string & name, int constantSize)
{
    StructUnion s;
    s.name = name;
    s.owner = owner;
    s.sizeFUCK = constantSize;
    return addStructUnion(s);
}

bool TypeManager::AddUnion(const std::string & owner, const std::string & name, int constantSize)
{
    StructUnion u;
    u.owner = owner;
    u.name = name;
    u.isUnion = true;
    u.sizeFUCK = constantSize;
    return addStructUnion(u);
}

bool TypeManager::AddEnum(const std::string & owner, const std::string & name, bool isFlags, uint8_t size)
{
    struct Enum e;
    e.owner = owner;
    e.name = name;
    e.members = { };
    e.isFlags = isFlags;
    e.sizeFUCK = size;

    if(enums.find(name) != enums.end())
        return false;

    enums.insert({ e.name, e });
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

bool TypeManager::AddStructMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize, int bitOffset, int bitSize, bool isBitfield)
{
    if(!isDefined(type) && !validPtr(type))
        return false;
    auto found = structs.find(parent);
    if(arrsize < 0 || found == structs.end() || !isDefined(type) || name.empty() || type.empty() || type == parent)
        return false;

    // cannot have bit field array
    if(isBitfield && arrsize != 0)
        return false;

    // cannot be pointer and bitfield
    int typeSize;
    if(type.back() == '*')
    {
        // this is a pointer type
        auto expectedSize = primitivesizes[Pointer] * 8;
        if(arrsize != 0)
            expectedSize *= arrsize;

        if(bitSize != expectedSize)
            return false;

        typeSize = expectedSize;
    }
    else
    {
        if(arrsize != 0)
            typeSize = Sizeof(type) * arrsize;
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
        if(bitSize > typeSize)
            return false;

        typeSize = bitSize;
    }

    if(typeSize == -1)
        __debugbreak();

    Member m;
    m.name = name;
    m.arrsize = arrsize;
    m.type = type;
    m.assignedType = type;
    m.offsetFUCK = bitOffset;
    m.bitSize = typeSize;
    m.bitfield = isBitfield;

    if(bitOffset >= 0 && !s.isUnion)  //user-defined offset
    {
        if(bitOffset < s.sizeFUCK)
            return false;

        if(bitOffset > s.sizeFUCK)
            AppendStructPadding(parent, bitOffset);
    }

    if(s.isUnion)
    {
        if(typeSize > s.sizeFUCK)
            s.sizeFUCK = typeSize;
    }
    else
    {
        s.sizeFUCK += m.bitSize;
    }

    s.members.push_back(m);
    return true;
}

bool TypeManager::AppendStructMember(const std::string & type, const std::string & name, int arrsize, int offset)
{
    return AddStructMember(laststruct, type, name, arrsize, offset, -1, false);
}

bool TypeManager::AppendStructPadding(const std::string & parent, int targetOffset)
{
    auto found = structs.find(parent);
    if(found == structs.end())
        return false;

    auto & s = found->second;
    if(s.sizeFUCK >= targetOffset || s.isUnion)
        return false;

    auto bitPadding = targetOffset - s.sizeFUCK;
    if(bitPadding % 8 != 0)
    {
        const auto remBits = bitPadding % 8;

        Member pad;
        pad.type = "long long";
        pad.bitSize = remBits;

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
        pad.bitSize = bitPadding;
        pad.arrsize = bitPadding / 8;

        // can just add byte padding
        char padName[32] = { };
        sprintf_s(padName, "padding%d", bitPadding / 8);

        pad.name = padName;
        s.members.push_back(pad);
    }

    s.sizeFUCK = targetOffset;
    return true;
}

bool TypeManager::AddFunction(const std::string & owner, const std::string & name, CallingConvention callconv,
                              bool noreturn)
{
    auto found = functions.find(name);
    if(found != functions.end() || name.empty() || owner.empty())
        return false;
    lastfunction = name;
    Function f;
    f.owner = owner;
    f.name = name;
    f.rettype = "";

    // return type cannot be set yet

    f.callconv = callconv;
    f.noreturn = noreturn;
    functions.insert({ f.name, f });
    return true;
}

bool TypeManager::AddFunctionReturn(const std::string & name, const std::string & rettype)
{
    auto found = functions.find(name);
    if(found == functions.end() || name.empty())
        return false;

    Function & f = found->second;
    if(rettype != "void" && !isDefined(rettype) && !validPtr(rettype))
        return false;
    f.rettype = rettype;
    return true;
}

bool TypeManager::AddArg(const std::string & function, const std::string & type, const std::string & name)
{
    if(!isDefined(type) && !validPtr(type))
        return false;
    auto found = functions.find(function);
    if(found == functions.end() || function.empty() || name.empty() || !isDefined(type))
        return false;
    lastfunction = function;
    Member arg;
    arg.name = name;
    arg.type = type;
    arg.assignedType = type;
    found->second.args.push_back(arg);
    return true;
}

bool TypeManager::AppendArg(const std::string & type, const std::string & name)
{
    return AddArg(lastfunction, type, name);
}

int TypeManager::Sizeof(const std::string & type, std::string* underlyingType)
{
    if(!types.empty() && type.back() == '*')
    {
        if(underlyingType != nullptr) *underlyingType = type;
        return primitivesizes[Pointer] * 8;
    }

    auto foundT = types.find(type);
    if(foundT != types.end())
    {
        if(!foundT->second.pointto.empty())
            return Sizeof(foundT->second.pointto, underlyingType);

        if(underlyingType != nullptr) *underlyingType = foundT->second.name;
        return foundT->second.sizeFUCK;
    }

    auto foundE = enums.find(type);
    if(foundE != enums.end())
    {
        if(underlyingType != nullptr) *underlyingType = foundE->second.name;
        return foundE->second.sizeFUCK;
    }

    auto foundS = structs.find(type);
    if(foundS != structs.end())
    {
        if(underlyingType != nullptr) *underlyingType = foundS->second.name;
        return foundS->second.sizeFUCK;
    }

    auto foundF = functions.find(type);
    if(foundF != functions.end())
    {
        if(underlyingType != nullptr) *underlyingType = foundF->second.name;
        return primitivesizes[Pointer] * 8;
    }

    return 0;
}

Enum TypeManager::TypeEnumData(const std::string & type)
{
    const auto enumT = enums.find(type);
    if(enumT != enums.end())
        return enumT->second;

    return { };
}

bool TypeManager::Visit(const std::string & type, const std::string & name, Visitor & visitor) const
{
    Member m;
    m.name = name;
    m.type = type;
    m.assignedType = type;

    return visitMember(m, visitor);
}

template <typename K, typename V>
static void filterOwnerMap(std::unordered_map<K, V> & map, const std::string & owner)
{
    for(auto i = map.begin(); i != map.end();)
    {
        auto j = i++;
        if(j->second.owner.empty())
            continue;
        if(owner.empty() || j->second.owner == owner)
            map.erase(j);
    }
}

void TypeManager::Clear(const std::string & owner)
{
    laststruct.clear();
    lastfunction.clear();
    filterOwnerMap(types, owner);
    filterOwnerMap(structs, owner);
    filterOwnerMap(functions, owner);
    filterOwnerMap(enums, owner);
}

template <typename K, typename V>
static bool removeType(std::unordered_map<K, V> & map, const std::string & type)
{
    auto found = map.find(type);
    if(found == map.end())
        return false;
    if(found->second.owner.empty())
        return false;
    map.erase(found);
    return true;
}

bool TypeManager::RemoveType(const std::string & type)
{
    return removeType(types, type) || removeType(structs, type) || removeType(functions, type);
}

static std::string getKind(const StructUnion & su)
{
    return su.isUnion ? "union" : "struct";
}

static std::string getKind(const Type & t)
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

std::string Types::TypeManager::StructUnionPtrType(const std::string & pointto) const
{
    auto itr = structs.find(pointto);
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
    laststruct = s.name;
    if(s.owner.empty() || s.name.empty() || isDefined(s.name))
        return false;
    structs.insert({ s.name, s });
    return true;
}

bool TypeManager::addType(const Type & t)
{
    if(t.name.empty() || isDefined(t.name))
        return false;
    types.insert({ t.name, t });
    return true;
}

bool TypeManager::addType(const std::string & owner, Primitive primitive, const std::string & name, const std::string & pointto)
{
    auto foundT = types.find(name);
    if(foundT != types.end())
    {
        if(pointto.empty())
        {
            if(primitivesizes[primitive] == primitivesizes[foundT->second.primitive])
                return true;
        }
        else
        {
            std::string underlyingType;
            Sizeof(pointto, &underlyingType);

            if(underlyingType == pointto)
                return true;
        }
    }

    if(name.empty() || isDefined(name))
        return false;
    Type t;
    t.owner = owner;
    t.name = name;
    t.primitive = primitive;
    t.sizeFUCK = primitivesizes[primitive] * 8;
    t.pointto = pointto;
    return addType(t);
}

bool TypeManager::visitMember(const Member & root, Visitor & visitor) const
{
    auto foundT = types.find(root.type);
    if(foundT != types.end())
    {
        const auto & t = foundT->second;
        if(t.primitive == Typedef)  // check if struct type
        {
            Member member = root;
            member.type = t.pointto;

            return visitMember(member, visitor);
        }

        if(!t.pointto.empty())
        {
            if(!isDefined(t.pointto))
                return false;
            if(visitor.visitPtr(root, t))  //allow the visitor to bail out
            {
                if(!Visit(t.pointto, "*" + root.name, visitor))
                    return false;
                return visitor.visitBack(root);
            }
            return true;
        }

        return visitor.visitType(root, t);
    }

    auto foundS = structs.find(root.type);
    if(foundS != structs.end())
    {
        const auto & s = foundS->second;
        if(!visitor.visitStructUnion(root, s))
            return false;

        for(const auto & child : s.members)
        {
            if(child.arrsize)
            {
                if(!visitor.visitArray(child))
                    return false;

                Member am = child;
                am.bitSize = child.bitSize / child.arrsize;
                am.arrsize = -1;

                for(auto i = 0; i < child.arrsize; i++)
                    if(!visitMember(am, visitor))
                        return false;

                if(!visitor.visitBack(child))
                    return false;
            }
            else if(!visitMember(child, visitor))
                return false;
        }
        return visitor.visitBack(root);
    }

    auto foundE = enums.find(root.type);
    if(foundE != enums.end())
    {
        const auto & e = foundE->second;
        return visitor.visitEnum(root, e);
    }

    auto foundF = functions.find(root.type);
    if(foundF != functions.end())
        return true;

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

bool AddMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize, int offset)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddStructMember(parent, type, name, arrsize, offset, -1, false);
}

bool AppendMember(const std::string & type, const std::string & name, int arrsize, int offset)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AppendStructMember(type, name, arrsize, offset);
}

bool AddFunction(const std::string & owner, const std::string & name, const std::string & rettype, Types::CallingConvention callconv, bool noreturn)
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

Enum TypeEnumData(const std::string & type)
{
    SHARED_ACQUIRE(LockTypeManager);
    return typeManager.TypeEnumData(type);
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
            size = json_default_int(vali, "bitSize", 8);
        else
            size *= 8;

        curSu.sizeFUCK = size;

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
            curMember.arrsize = json_default_int(valj, "arrsize", 0);
            size = json_default_int(valj, "size", -1);
            if(size == -1)
                size = json_default_int(valj, "bitSize", 8);
            else
                size *= 8;
            curMember.bitSize = size;
            curMember.bitfield = json_boolean_value(json_object_get(valj, "bitfield"));

            curMember.offsetFUCK = json_default_int(valj, "offset", -1);
            if(curMember.offsetFUCK == -1)
                curMember.offsetFUCK = json_default_int(valj, "bitOffset", -1);
            else
                curMember.offsetFUCK *= 8;

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
        auto rettype = json_string_value(json_object_get(vali, "rettype"));
        auto fname = json_string_value(json_object_get(vali, "name"));
        if(!rettype || !*rettype || !fname || !*fname)
            continue;
        curFunction.rettype = rettype;
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
            size = json_default_int(vali, "bitSize", 8);
        else
            size *= 8;

        curE.sizeFUCK = size;
        curE.isFlags = json_boolean(vali, "isFlags");

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

void LoadModel(const std::string & owner, Model & model)
{
    //Add all base struct/union types first to avoid errors later
    for(auto & su : model.structUnions)
    {
        auto success = su.isUnion ? typeManager.AddUnion(owner, su.name, 0) : typeManager.AddStruct(owner, su.name, 0);
        if(!success)
        {
            //TODO properly handle errors
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
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add function %s %s()\n"), function.rettype.c_str(), function.name.c_str());
            function.name.clear(); //signal error
        }
    }

    for(auto & num : model.enums)
    {
        if(num.name.empty())  //skip error-signalled functions
            continue;

        auto success = typeManager.AddEnum(owner, num.name, num.isFlags, num.sizeFUCK);
        if(!success)
        {
            //TODO properly handle errors
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
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add typedef %s %s;\n"), type.type.c_str(), type.name.c_str());
        }
    }

    //Add struct/union members
    for(auto & su : model.structUnions)
    {
        if(su.name.empty())  //skip error-signalled structs/unions
            continue;

        const auto suggestedSize = su.sizeFUCK;
        for(auto & member : su.members)
        {
            auto success = typeManager.AddStructMember(su.name, member.type, member.name, member.arrsize, member.offsetFUCK, member.bitSize, member.bitfield);
            if(!success)
            {
                //TODO properly handle errors
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
                dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add enum member %s\n"), mem.second.c_str());
            }
        }
    }

    //Add function arguments
    for(auto & function : model.functions)
    {
        if(function.name.empty())  //skip error-signalled functions
            continue;

        bool status = typeManager.AddFunctionReturn(function.name, function.rettype);
        if(!status)
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add return type %s.%s;\n"), function.name.c_str(), function.rettype.c_str());
        }

        for(auto & arg : function.args)
        {
            auto success = typeManager.AddArg(function.name, arg.type, arg.name);
            if(!success)
            {
                //TODO properly handle errors
                dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add argument %s %s.%s;\n"), arg.type.c_str(), function.name.c_str(), arg.name.c_str());
            }
        }
    }
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

std::string StructUnionPtrType(const std::string & pointto)
{
    return typeManager.StructUnionPtrType(pointto);
}
