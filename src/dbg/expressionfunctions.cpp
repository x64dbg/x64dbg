#include "expressionfunctions.h"
#include "threading.h"
#include "exprfunc.h"
#include "module.h"

std::unordered_map<String, ExpressionFunctions::Function> ExpressionFunctions::mFunctions;

//Copied from http://stackoverflow.com/a/7858971/1806760
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
    return ExpressionFunctions::Register(name, sizeof...(Ts), [cbFunction](int argc, const duint * argv)
    {
        return callFunc(argv, cbFunction, typename gens<sizeof...(Ts)>::type());
    });
}

void ExpressionFunctions::Init()
{
    //TODO: register more functions
    using namespace Exprfunc;

    //undocumented
    RegisterEasy("src.line", srcline);
    RegisterEasy("src.disp", srcdisp);

    RegisterEasy("mod.party", modparty);
    RegisterEasy("mod.base", ModBaseFromAddr);
    RegisterEasy("mod.size", ModSizeFromAddr);
    RegisterEasy("mod.hash", ModHashFromAddr);
    RegisterEasy("mod.entry", ModEntryFromAddr);
}

duint ExpressionFunctions::Function::Invoke(int argc, const duint* argv) const
{
    if(cbFunction)
    {
        return cbFunction(argc, argv);
    }
    else if(cbFunctionWithUserData)
    {
        return cbFunctionWithUserData(argc, argv, userdata);
    }

    return 0;
}
bool ExpressionFunctions::Register(const String & name, int argc, CBEXPRESSIONFUNCTIONWITHUSERDATA cbFunction, void* user)
{
    if(!isValidName(name))
        return false;
    EXCLUSIVE_ACQUIRE(LockExpressionFunctions);
    if(mFunctions.count(name))
        return false;
    Function f;
    f.name = name;
    f.argc = argc;
    f.cbFunctionWithUserData = cbFunction;
    f.userdata = user;
    mFunctions[name] = f;
    return true;
}
bool ExpressionFunctions::Register(const String & name, int argc, CBEXPRESSIONFUNCTION cbFunction)
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
    mFunctions[name] = f;
    return true;
}

bool ExpressionFunctions::Unregister(const String & name)
{
    EXCLUSIVE_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    mFunctions.erase(found);
    return true;
}

bool ExpressionFunctions::Call(const String & name, const std::vector<duint> & argv, duint & result)
{
    SHARED_ACQUIRE(LockExpressionFunctions);
    auto found = mFunctions.find(name);
    if(found == mFunctions.end())
        return false;
    const auto & f = found->second;
    if(f.argc != int(argv.size()))
        return false;
    result = f.Invoke(f.argc, argv.data());
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
