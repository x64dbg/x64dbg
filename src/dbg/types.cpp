#include "types.h"
#include "stringutils.h"
#include "threading.h"

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
    return addType(owner, found->second.primitive, name);
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

    if(offset >= 0)  //user-defined offset
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
            s.members.push_back(m);
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

template<typename K, typename V>
static void enumType(const std::unordered_map<K, V> & map, const std::string & kind, std::vector<TypeManager::Summary> & types)
{
    for(auto i = map.begin(); i != map.end(); ++i)
    {
        TypeManager::Summary s;
        s.kind = kind;
        s.name = i->second.name;
        s.owner = i->second.owner;
        s.size = SizeofType(s.name);
        types.push_back(s);
    }
}

void TypeManager::Enum(std::vector<Summary> & typeList) const
{
    typeList.clear();
    enumType(types, "typedef", typeList);
    enumType(structs, "structunion", typeList);
    enumType(functions, "function", typeList);
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
        if(!isDefined(type))
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
