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
    p("double", Double, sizeof(double));
    p("ptr,void*", Pointer, sizeof(void*));
    p("char*,const char*", PtrString, sizeof(char*));
    p("wchar_t*,const wchar_t*", PtrWString, sizeof(wchar_t*));
}

bool TypeManager::AddType(const std::string & owner, const std::string & type, const std::string & name)
{
    if(owner.empty())
        return false;
    validPtr(type);
    auto found = types.find(type);
    if(found == types.end())
        return false;
    return addType(owner, found->second.primitive, name, found->second.pointto);
}

bool TypeManager::AddStruct(const std::string & owner, const std::string & name)
{
    StructUnion s;
    s.name = name;
    s.owner = owner;
    return addStructUnion(s);
}

bool TypeManager::AddUnion(const std::string & owner, const std::string & name)
{
    StructUnion u;
    u.owner = owner;
    u.name = name;
    u.isunion = true;
    return addStructUnion(u);
}

bool TypeManager::AddMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize, int offset)
{
    if(!isDefined(type) && !validPtr(type))
        return false;
    auto found = structs.find(parent);
    if(arrsize < 0 || found == structs.end() || !isDefined(type) || name.empty() || type.empty() || type == parent)
        return false;
    auto & s = found->second;

    for(const auto & member : s.members)
        if(member.name == name)
            return false;

    auto typeSize = Sizeof(type);
    if(arrsize)
        typeSize *= arrsize;

    Member m;
    m.name = name;
    m.arrsize = arrsize;
    m.type = type;
    m.offset = offset;

    if(offset >= 0) //user-defined offset
    {
        if(offset < s.size)
            return false;
        if(offset > s.size)
        {
            Member pad;
            pad.type = "char";
            pad.arrsize = offset - s.size;
            char padname[32] = "";
            sprintf_s(padname, "padding%d", pad.arrsize);
            pad.name = padname;
            s.members.push_back(pad);
            s.size += pad.arrsize;
        }
    }

    s.members.push_back(m);

    if(s.isunion)
    {
        if(typeSize > s.size)
            s.size = typeSize;
    }
    else
    {
        s.size += typeSize;
    }
    return true;
}

bool TypeManager::AppendMember(const std::string & type, const std::string & name, int arrsize, int offset)
{
    return AddMember(laststruct, type, name, arrsize, offset);
}

bool TypeManager::AddFunction(const std::string & owner, const std::string & name, const std::string & rettype, CallingConvention callconv, bool noreturn)
{
    auto found = functions.find(name);
    if(found != functions.end() || name.empty() || owner.empty())
        return false;
    lastfunction = name;
    Function f;
    f.owner = owner;
    f.name = name;
    if(rettype != "void" && !isDefined(rettype) && !validPtr(rettype))
        return false;
    f.rettype = rettype;
    f.callconv = callconv;
    f.noreturn = noreturn;
    functions.insert({f.name, f});
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
    found->second.args.push_back(arg);
    return true;
}

bool TypeManager::AppendArg(const std::string & type, const std::string & name)
{
    return AddArg(lastfunction, type, name);
}

int TypeManager::Sizeof(const std::string & type) const
{
    auto foundT = types.find(type);
    if(foundT != types.end())
        return foundT->second.size;
    auto foundS = structs.find(type);
    if(foundS != structs.end())
        return foundS->second.size;
    auto foundF = functions.find(type);
    if(foundF != functions.end())
    {
        const auto foundP = primitivesizes.find(Pointer);
        if(foundP != primitivesizes.end())
            return foundP->second;
        return sizeof(void*);
    }
    return 0;
}

bool TypeManager::Visit(const std::string & type, const std::string & name, Visitor & visitor) const
{
    Member m;
    m.name = name;
    m.type = type;
    return visitMember(m, visitor);
}

template<typename K, typename V>
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
}

template<typename K, typename V>
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
    return su.isunion ? "union" : "struct";
}

static std::string getKind(const Type & t)
{
    return "typedef";
}

static std::string getKind(const Function & f)
{
    return "function";
}

template<typename K, typename V>
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

template<typename K, typename V>
static bool mapContains(const std::unordered_map<K, V> & map, const K & k)
{
    return map.find(k) != map.end();
}

bool TypeManager::isDefined(const std::string & id) const
{
    return mapContains(types, id) || mapContains(structs, id);
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
        return addType(owner, Pointer, id, type);
    }
    return false;
}

bool TypeManager::addStructUnion(const StructUnion & s)
{
    laststruct = s.name;
    if(s.owner.empty() || s.name.empty() || isDefined(s.name))
        return false;
    structs.insert({s.name, s});
    return true;
}

bool TypeManager::addType(const Type & t)
{
    if(t.name.empty() || isDefined(t.name))
        return false;
    types.insert({t.name, t});
    return true;
}

bool TypeManager::addType(const std::string & owner, Primitive primitive, const std::string & name, const std::string & pointto)
{
    if(name.empty() || isDefined(name))
        return false;
    Type t;
    t.owner = owner;
    t.name = name;
    t.primitive = primitive;
    t.size = primitivesizes[primitive];
    t.pointto = pointto;
    return addType(t);
}

bool TypeManager::visitMember(const Member & root, Visitor & visitor) const
{
    auto foundT = types.find(root.type);
    if(foundT != types.end())
    {
        const auto & t = foundT->second;
        if(!t.pointto.empty())
        {
            if(!isDefined(t.pointto))
                return false;
            if(visitor.visitPtr(root, t)) //allow the visitor to bail out
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
                for(auto i = 0; i < child.arrsize; i++)
                    if(!visitMember(child, visitor))
                        return false;
                if(!visitor.visitBack(child))
                    return false;
            }
            else if(!visitMember(child, visitor))
                return false;
        }
        return visitor.visitBack(root);
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

bool AddMember(const std::string & parent, const std::string & type, const std::string & name, int arrsize, int offset)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddMember(parent, type, name, arrsize, offset);
}

bool AppendMember(const std::string & type, const std::string & name, int arrsize, int offset)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AppendMember(type, name, arrsize, offset);
}

bool AddFunction(const std::string & owner, const std::string & name, const std::string & rettype, Types::CallingConvention callconv, bool noreturn)
{
    EXCLUSIVE_ACQUIRE(LockTypeManager);
    return typeManager.AddFunction(owner, name, rettype, callconv, noreturn);
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

static void loadStructUnions(const JSON suroot, bool isunion, std::vector<StructUnion> & structUnions)
{
    if(!suroot)
        return;
    size_t i;
    JSON vali;
    StructUnion curSu;
    curSu.isunion = isunion;
    json_array_foreach(suroot, i, vali)
    {
        auto suname = json_string_value(json_object_get(vali, "name"));
        if(!suname || !*suname)
            continue;
        curSu.name = suname;
        curSu.members.clear();
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
            curMember.offset = json_default_int(valj, "offset", -1);
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

void LoadModel(const std::string & owner, Model & model)
{
    //Add all base struct/union types first to avoid errors later
    for(auto & su : model.structUnions)
    {
        auto success = su.isunion ? typeManager.AddUnion(owner, su.name) : typeManager.AddStruct(owner, su.name);
        if(!success)
        {
            //TODO properly handle errors
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add %s %s;\n"), su.isunion ? "union" : "struct", su.name.c_str());
            su.name.clear(); //signal error
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

    //Add base function types to avoid errors later
    for(auto & function : model.functions)
    {
        auto success = typeManager.AddFunction(owner, function.name, function.rettype, function.callconv, function.noreturn);
        if(!success)
        {
            //TODO properly handle errors
            dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add function %s %s()\n"), function.rettype.c_str(), function.name.c_str());
            function.name.clear(); //signal error
        }
    }

    //Add struct/union members
    for(auto & su : model.structUnions)
    {
        if(su.name.empty()) //skip error-signalled structs/unions
            continue;
        for(auto & member : su.members)
        {
            auto success = typeManager.AddMember(su.name, member.type, member.name, member.arrsize, member.offset);
            if(!success)
            {
                //TODO properly handle errors
                dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add member %s %s.%s;\n"), member.type.c_str(), su.name.c_str(), member.name.c_str());
            }
        }
    }

    //Add function arguments
    for(auto & function : model.functions)
    {
        if(function.name.empty()) //skip error-signalled functions
            continue;
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
    auto root = json_loads(json.c_str(), 0, 0);
    if(root)
    {
        Model model;
        loadTypes(json_object_get(root, "types"), model.types);
        loadTypes(json_object_get(root, ArchValue("types32", "types64")), model.types);
        loadStructUnions(json_object_get(root, "structs"), false, model.structUnions);
        loadStructUnions(json_object_get(root, ArchValue("structs32", "structs64")), false, model.structUnions);
        loadStructUnions(json_object_get(root, "unions"), true, model.structUnions);
        loadStructUnions(json_object_get(root, ArchValue("unions32", "unions64")), true, model.structUnions);
        loadFunctions(json_object_get(root, "functions"), model.functions);
        loadFunctions(json_object_get(root, ArchValue("functions32", "functions64")), model.functions);

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
