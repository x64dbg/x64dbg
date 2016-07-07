#include "expressionfunctions.h"
#include "threading.h"

std::unordered_map<String, ExpressionFunctions::Function> ExpressionFunctions::mFunctions;

void ExpressionFunctions::Init()
{
    //TODO: register some functions
}

bool ExpressionFunctions::Register(const String & name, int argc, CBEXPRESSIONFUNCTION cbFunction)
{
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
    result = f.cbFunction(f.argc, argv.data());
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
