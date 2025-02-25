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
static duint callFunc(const T* argv, duint(*cbFunction)(Ts...), seq<S...>)
{
    return cbFunction(argv[S].number...);
}

template<typename... Ts>
static bool RegisterEasy(const String & name, duint(*cbFunction)(Ts...))
{
    auto tempFunc = [cbFunction](ExpressionValue * result, int argc, const ExpressionValue * argv, void* userdata) -> bool
    {
        result->type = ValueTypeNumber;
        result->number = callFunc(argv, cbFunction, typename gens<sizeof...(Ts)>::type());

        return true;
    };
    std::vector<ValueType> args(sizeof...(Ts));

    for(auto & arg : args)
        arg = ValueTypeNumber;

    return ExpressionFunctions::Register(name, ValueTypeNumber, args, tempFunc);
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
    ExpressionFunctions::Register("mod.fromname", ValueTypeNumber, { ValueTypeString }, Exprfunc::modbasefromname);

    //Process information
    RegisterEasy("peb,PEB", peb);
    RegisterEasy("teb,TEB", teb);
    RegisterEasy("tid,TID,ThreadId", tid);
    RegisterEasy("kusd,KUSD,KUSER_SHARED_DATA", kusd);

    //General purpose
    RegisterEasy("bswap", bswap);
    ExpressionFunctions::Register("ternary,tern", ValueTypeAny, { ValueTypeNumber, ValueTypeAny, ValueTypeAny }, ternary);
    RegisterEasy("GetTickCount,gettickcount", gettickcount);
    RegisterEasy("rdtsc", rdtsc);

    //Memory
    RegisterEasy("mem.valid,mem.isvalid", memvalid);
    RegisterEasy("mem.base", membase);
    RegisterEasy("mem.size", memsize);
    RegisterEasy("mem.iscode", memiscode);
    RegisterEasy("mem.isstring", memisstring);
    RegisterEasy("mem.decodepointer", memdecodepointer);
    ExpressionFunctions::Register("mem.match", ValueTypeNumber, {ValueTypeNumber, ValueTypeString}, memmatch);

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
    RegisterEasy("dis.iscallsystem", disiscallsystem);
    ExpressionFunctions::Register("dis.mnemonic", ValueTypeString, { ValueTypeNumber }, dismnemonic);
    ExpressionFunctions::Register("dis.text", ValueTypeString, { ValueTypeNumber }, distext);
    ExpressionFunctions::Register("dis.match", ValueTypeNumber, { ValueTypeNumber, ValueTypeString }, dismatch);

    //Trace record
    RegisterEasy("tr.enabled", trenabled);
    RegisterEasy("tr.hitcount,tr.count", trhitcount);
    RegisterEasy("tr.isrecording,tr.runtraceenabled", trisrecording);

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

    //Exceptions
    RegisterEasy("ex.firstchance", exfirstchance);
    RegisterEasy("ex.addr", exaddr);
    RegisterEasy("ex.code", excode);
    RegisterEasy("ex.flags", exflags);
    RegisterEasy("ex.infocount", exinfocount);
    RegisterEasy("ex.info", exinfo);

    //Undocumented
    RegisterEasy("bpgoto", bpgoto);

    //Other
    RegisterEasy("isdebuggerfocused", isdebuggerfocused);
    RegisterEasy("isdebuggeefocused", isdebuggeefocused);

    // Strings
    ExpressionFunctions::Register("ansi", ValueTypeString, { ValueTypeNumber, ValueTypeOptionalNumber }, Exprfunc::ansi);
    ExpressionFunctions::Register("ansi.strict", ValueTypeString, { ValueTypeNumber, ValueTypeOptionalNumber }, Exprfunc::ansi_strict);
    ExpressionFunctions::Register("utf8", ValueTypeString, { ValueTypeNumber, ValueTypeOptionalNumber }, Exprfunc::utf8);
    ExpressionFunctions::Register("utf8.strict", ValueTypeString, { ValueTypeNumber, ValueTypeOptionalNumber }, Exprfunc::utf8_strict);
    ExpressionFunctions::Register("utf16", ValueTypeString, { ValueTypeNumber, ValueTypeOptionalNumber }, Exprfunc::utf16);
    ExpressionFunctions::Register("utf16.strict", ValueTypeString, { ValueTypeNumber, ValueTypeOptionalNumber }, Exprfunc::utf16_strict);
    ExpressionFunctions::Register("strstr", ValueTypeNumber, { ValueTypeString, ValueTypeString }, Exprfunc::strstr);
    ExpressionFunctions::Register("stristr", ValueTypeNumber, { ValueTypeString, ValueTypeString }, Exprfunc::stristr);
    ExpressionFunctions::Register("streq", ValueTypeNumber, { ValueTypeString, ValueTypeString }, Exprfunc::streq);
    ExpressionFunctions::Register("strieq", ValueTypeNumber, { ValueTypeString, ValueTypeString }, Exprfunc::strieq);
    ExpressionFunctions::Register("strlen", ValueTypeNumber, { ValueTypeString }, Exprfunc::strlen);

    ExpressionFunctions::Register("syscall.name", ValueTypeString, { ValueTypeNumber }, Exprfunc::syscall_name);
    ExpressionFunctions::Register("syscall.id", ValueTypeNumber, { ValueTypeString }, Exprfunc::syscall_id);
}

bool ExpressionFunctions::Register(const String & name, const ValueType & returnType, const std::vector<ValueType> & argTypes, const CBEXPRESSIONFUNCTION & cbFunction, void* userdata)
{
    EXCLUSIVE_ACQUIRE(LockExpressionFunctions);
    auto aliases = StringUtils::Split(name, ',');
    if(!isValidName(aliases[0]))
        return false;
    if(mFunctions.count(aliases[0]))
        return false;

    // Return type cannot be optional
    switch(returnType)
    {
    case ValueTypeOptionalNumber:
    case ValueTypeOptionalString:
    case ValueTypeOptionalAny:
        return false;
    default:
        break;
    }

    // Make sure optional arguments are at the end
    bool seenOptional = false;
    for(const auto & argType : argTypes)
    {
        switch(argType)
        {
        case ValueTypeOptionalNumber:
        case ValueTypeOptionalString:
        case ValueTypeOptionalAny:
            seenOptional = true;
            break;
        default:
            if(seenOptional)
                return false;
            break;
        }
    }

    Function f;
    f.name = aliases[0];
    f.returnType = returnType;
    f.argTypes = argTypes;
    f.cbFunction = cbFunction;
    f.userdata = userdata;
    mFunctions[aliases[0]] = f;

    for(size_t i = 1; i < aliases.size(); i++)
        ExpressionFunctions::RegisterAlias(aliases[0], aliases[i]);

    return true;
}

bool ExpressionFunctions::RegisterAlias(const String & name, const String & alias)
{
    EXCLUSIVE_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;

    if(!Register(alias, found->second.returnType, found->second.argTypes, found->second.cbFunction, found->second.userdata))
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
    for(const auto & alias : aliases)
        Unregister(alias);
    return true;
}

bool ExpressionFunctions::Call(const String & name, ExpressionValue & result, std::vector<ExpressionValue> & argv)
{
    SHARED_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    const auto & f = found->second;
    return f.cbFunction(&result, (int)argv.size(), argv.data(), f.userdata);
}

bool ExpressionFunctions::GetType(const String & name, ValueType & returnType, std::vector<ValueType> & argTypes)
{
    SHARED_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    returnType = found->second.returnType;
    argTypes = found->second.argTypes;
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
