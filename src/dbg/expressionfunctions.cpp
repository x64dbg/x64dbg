#include "expressionfunctions.h"
#include "threading.h"
#include "exprfunc.h"
#include "module.h"
#include "debugger.h"
#include "value.h"

std::unordered_map<String, ExpressionFunctions::Function> ExpressionFunctions::mFunctions;

//Copied from https://stackoverflow.com/a/7858971/1806760
template<int...>
struct seq {};

template<int N, int... S>
struct gens : gens < N - 1, N - 1, S... > {};

template<int... S>
struct gens<0, S...>
{
    typedef seq<S...> type;
};

template<typename T, int ...S, typename... Ts>
static T callFunc(const T* argv, T(*cbFunction)(Ts...), seq<S...>)
{
    return cbFunction(argv[S]...);
}

template<typename... Ts>
static bool RegisterEasy(const String & name, duint(*cbFunction)(Ts...))
{
    auto aliases = StringUtils::Split(name, ',');
    auto tempFunc = [cbFunction](int argc, duint * argv, void* userdata)
    {
        return callFunc(argv, cbFunction, typename gens<sizeof...(Ts)>::type());
    };
    if(!ExpressionFunctions::Register(aliases[0], sizeof...(Ts), tempFunc))
        return false;
    for(size_t i = 1; i < aliases.size(); i++)
        ExpressionFunctions::RegisterAlias(aliases[0], aliases[i]);
    return true;
}

void ExpressionFunctions::Init()
{
    //TODO: register more functions
    using namespace Exprfunc;

    //GUI interaction
    RegisterEasy("disasm.sel,dis.sel", disasmsel);
    RegisterEasy("dump.sel", dumpsel);
    RegisterEasy("stack.sel", stacksel);

    //Source
    RegisterEasy("src.line", srcline);
    RegisterEasy("src.disp", srcdisp);

    //Modules
    RegisterEasy("mod.party", modparty);
    RegisterEasy("mod.base", ModBaseFromAddr);
    RegisterEasy("mod.size", ModSizeFromAddr);
    RegisterEasy("mod.hash", ModHashFromAddr);
    RegisterEasy("mod.entry", ModEntryFromAddr);
    RegisterEasy("mod.system,mod.issystem", modsystem);
    RegisterEasy("mod.user,mod.isuser", moduser);
    RegisterEasy("mod.main,mod.mainbase", dbgdebuggedbase);
    RegisterEasy("mod.rva", modrva);
    RegisterEasy("mod.offset,mod.fileoffset", valvatofileoffset);
    RegisterEasy("mod.headerva", modheaderva);
    RegisterEasy("mod.isexport", modisexport);

    //Process information
    RegisterEasy("peb,PEB", peb);
    RegisterEasy("teb,TEB", teb);
    RegisterEasy("tid,TID,ThreadId", tid);

    //General purpose
    RegisterEasy("bswap", bswap);
    RegisterEasy("ternary,tern", ternary);
    RegisterEasy("GetTickCount,gettickcount", gettickcount);

    //Memory
    RegisterEasy("mem.valid,mem.isvalid", memvalid);
    RegisterEasy("mem.base", membase);
    RegisterEasy("mem.size", memsize);
    RegisterEasy("mem.iscode", memiscode);
    RegisterEasy("mem.isstring", memisstring);
    RegisterEasy("mem.decodepointer", memdecodepointer);

    //Disassembly
    RegisterEasy("dis.len,dis.size", dislen);
    RegisterEasy("dis.iscond", disiscond);
    RegisterEasy("dis.isbranch", disisbranch);
    RegisterEasy("dis.isret", disisret);
    RegisterEasy("dis.iscall", disiscall);
    RegisterEasy("dis.ismem", disismem);
    RegisterEasy("dis.isnop", disisnop);
    RegisterEasy("dis.isunusual", disisunusual);
    RegisterEasy("dis.branchdest", disbranchdest);
    RegisterEasy("dis.branchexec", disbranchexec);
    RegisterEasy("dis.imm", disimm);
    RegisterEasy("dis.brtrue", disbrtrue);
    RegisterEasy("dis.brfalse", disbrfalse);
    RegisterEasy("dis.next", disnext);
    RegisterEasy("dis.prev", disprev);

    //Trace record
    RegisterEasy("tr.enabled", trenabled);
    RegisterEasy("tr.hitcount,tr.count", trhitcount);
    RegisterEasy("tr.runtraceenabled", trisruntraceenabled);

    //Byte/Word/Dword/Qword/Pointer
    RegisterEasy("ReadByte,Byte,byte", readbyte);
    RegisterEasy("ReadWord,Word,word", readword);
    RegisterEasy("ReadDword,Dword,dword", readdword);
#ifdef _WIN64
    RegisterEasy("ReadQword,Qword,qword", readqword);
#endif //_WIN64
    RegisterEasy("ReadPtr,ReadPointer,ptr,Pointer,pointer", readptr);

    //Functions
    RegisterEasy("func.start,sub.start", funcstart);
    RegisterEasy("func.end,sub.end", funcend);

    //References
    RegisterEasy("ref.count", refcount);
    RegisterEasy("ref.addr", refaddr);
    RegisterEasy("refsearch.count", refsearchcount);
    RegisterEasy("refsearch.addr", refsearchaddr);

    //Arguments
    RegisterEasy("arg.get,arg", argget);
    RegisterEasy("arg.set", argset);

    //Undocumented
    RegisterEasy("bpgoto", bpgoto);
}

bool ExpressionFunctions::Register(const String & name, int argc, const CBEXPRESSIONFUNCTION & cbFunction, void* userdata)
{
    if(!isValidName(name))
        return false;
    EXCLUSIVE_ACQUIRE(LockExpressionFunctions);
    if(mFunctions.count(name))
        return false;
    Function f;
    f.name = name;
    f.argc = argc;
    f.cbFunction = cbFunction;
    f.userdata = userdata;
    mFunctions[name] = f;
    return true;
}

bool ExpressionFunctions::RegisterAlias(const String & name, const String & alias)
{
    EXCLUSIVE_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    if(!Register(alias, found->second.argc, found->second.cbFunction, found->second.userdata))
        return false;
    found->second.aliases.push_back(alias);
    return true;
}

bool ExpressionFunctions::Unregister(const String & name)
{
    EXCLUSIVE_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    auto aliases = found->second.aliases;
    mFunctions.erase(found);
    for(const auto & alias : found->second.aliases)
        Unregister(alias);
    return true;
}

bool ExpressionFunctions::Call(const String & name, std::vector<duint> & argv, duint & result)
{
    SHARED_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    const auto & f = found->second;
    if(f.argc != int(argv.size()))
        return false;
    result = f.cbFunction(f.argc, argv.data(), f.userdata);
    return true;
}

bool ExpressionFunctions::GetArgc(const String & name, int & argc)
{
    SHARED_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    argc = found->second.argc;
    return true;
}

bool ExpressionFunctions::isValidName(const String & name)
{
    if(!name.length())
        return false;
    if(!(name[0] == '_' || isalpha(name[0])))
        return false;
    for(const auto & ch : name)
        if(!(isalnum(ch) || ch == '_' || ch == '.'))
            return false;
    return true;
}
