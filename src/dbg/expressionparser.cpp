#include "expressionparser.h"
#include "value.h"
#include "console.h"
#include "variable.h"

ExpressionParser::Token::Associativity ExpressionParser::Token::associativity() const
{
    switch(mType)
    {
    case Type::OperatorUnarySub:
    case Type::OperatorNot:
    case Type::OperatorLogicalNot:
    case Type::OperatorAssign:
    case Type::OperatorAssignMul:
    case Type::OperatorAssignHiMul:
    case Type::OperatorAssignDiv:
    case Type::OperatorAssignMod:
    case Type::OperatorAssignAdd:
    case Type::OperatorAssignSub:
    case Type::OperatorAssignShl:
    case Type::OperatorAssignShr:
    case Type::OperatorAssignAnd:
    case Type::OperatorAssignXor:
    case Type::OperatorAssignOr:
    case Type::OperatorPrefixInc:
    case Type::OperatorPrefixDec:
        return Associativity::RightToLeft;
    case Type::OperatorMul:
    case Type::OperatorHiMul:
    case Type::OperatorDiv:
    case Type::OperatorMod:
    case Type::OperatorAdd:
    case Type::OperatorSub:
    case Type::OperatorShl:
    case Type::OperatorShr:
    case Type::OperatorAnd:
    case Type::OperatorXor:
    case Type::OperatorOr:
    case Type::OperatorEqual:
    case Type::OperatorNotEqual:
    case Type::OperatorBigger:
    case Type::OperatorSmaller:
    case Type::OperatorBiggerEqual:
    case Type::OperatorSmallerEqual:
    case Type::OperatorLogicalAnd:
    case Type::OperatorLogicalOr:
    case Type::OperatorLogicalImpl:
    case Type::OperatorSuffixInc:
    case Type::OperatorSuffixDec:
        return Associativity::LeftToRight;
    default:
        return Associativity::Unspecified;
    }
}

//As defined in http://en.cppreference.com/w/c/language/operator_precedence

int ExpressionParser::Token::precedence() const
{
    switch(mType)
    {
    case Type::OperatorSuffixInc:
    case Type::OperatorSuffixDec:
        return 1;
    case Type::OperatorUnarySub:
    case Type::OperatorNot:
    case Type::OperatorLogicalNot:
    case Type::OperatorPrefixInc:
    case Type::OperatorPrefixDec:
        return 2;
    case Type::OperatorMul:
    case Type::OperatorHiMul:
    case Type::OperatorDiv:
    case Type::OperatorMod:
        return 3;
    case Type::OperatorAdd:
    case Type::OperatorSub:
        return 4;
    case Type::OperatorShl:
    case Type::OperatorShr:
        return 5;
    case Type::OperatorSmaller:
    case Type::OperatorSmallerEqual:
    case Type::OperatorBigger:
    case Type::OperatorBiggerEqual:
        return 6;
    case Type::OperatorEqual:
    case Type::OperatorNotEqual:
        return 7;
    case Type::OperatorAnd:
        return 8;
    case Type::OperatorXor:
        return 9;
    case Type::OperatorOr:
        return 10;
    case Type::OperatorLogicalAnd:
        return 11;
    case Type::OperatorLogicalOr:
    case Type::OperatorLogicalImpl:
        return 12;
    case Type::OperatorAssign:
    case Type::OperatorAssignMul:
    case Type::OperatorAssignHiMul:
    case Type::OperatorAssignDiv:
    case Type::OperatorAssignMod:
    case Type::OperatorAssignAdd:
    case Type::OperatorAssignSub:
    case Type::OperatorAssignShl:
    case Type::OperatorAssignShr:
    case Type::OperatorAssignAnd:
    case Type::OperatorAssignXor:
    case Type::OperatorAssignOr:
        return 14;
    default:
        return 16;
    }
}

bool ExpressionParser::Token::isOperator() const
{
    return mType != Type::Data && mType != Type::OpenBracket && mType != Type::CloseBracket;
}

ExpressionParser::ExpressionParser(const String & expression)
    : mExpression(fixClosingBrackets(expression)),
      mIsValidExpression(true)
{
    tokenize();
    shuntingYard();
}

String ExpressionParser::fixClosingBrackets(const String & expression)
{
    size_t open = 0;
    size_t close = 0;
    auto len = expression.length();
    for(size_t i = 0; i < len; i++)
    {
        if(expression[i] == '(')
            open++;
        else if(expression[i] == ')')
            close++;
    }
    auto result = expression;
    if(close < open)
    {
        for(size_t i = 0; i < open - close; i++)
            result += ")";
    }
    return result;
}

void ExpressionParser::tokenize()
{
    size_t stateMemory = 0;
    auto len = mExpression.length();
    for(size_t i = 0; i < len; i++)
    {
        auto ch = mExpression[i];
        switch(ch)
        {
        case '[':
        {
            stateMemory++;
            mCurToken += ch;
        }
        break;

        case ']':
        {
            if(stateMemory)
                stateMemory--;
            mCurToken += ch;
        }
        break;

        default:
        {
            if(stateMemory)
                mCurToken += ch;
            else
            {
                switch(ch)
                {
                case '(':
                    addOperatorToken(ch, Token::Type::OpenBracket);
                    break;
                case ')':
                    addOperatorToken(ch, Token::Type::CloseBracket);
                    break;
                case '~':
                    addOperatorToken(ch, Token::Type::OperatorNot);
                    break;
                case '*':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("*=", Token::Type::OperatorAssignMul);
                    else
                        addOperatorToken(ch, Token::Type::OperatorMul);
                    break;
                case '`':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("`=", Token::Type::OperatorAssignHiMul);
                    else
                        addOperatorToken(ch, Token::Type::OperatorHiMul);
                    break;
                case '/':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("/=", Token::Type::OperatorAssignDiv);
                    else
                        addOperatorToken(ch, Token::Type::OperatorDiv);
                    break;
                case '%':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("%=", Token::Type::OperatorAssignMod);
                    else
                        addOperatorToken(ch, Token::Type::OperatorMod);
                    break;
                case '+':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("+=", Token::Type::OperatorAssignAdd);
                    else if(tryEatNextCh(i, '+'))
                        addOperatorToken("++", isUnaryOperator() ? Token::Type::OperatorPrefixInc : Token::Type::OperatorSuffixInc);
                    else if(!isUnaryOperator()) //ignore unary add operators
                        addOperatorToken(ch, Token::Type::OperatorAdd);
                    break;
                case '-':
                    if(tryEatNextCh(i, '>'))
                        addOperatorToken("->", Token::Type::OperatorLogicalImpl);
                    else if(tryEatNextCh(i, '='))
                        addOperatorToken("-=", Token::Type::OperatorAssignSub);
                    else if(tryEatNextCh(i, '-'))
                        addOperatorToken("--", isUnaryOperator() ? Token::Type::OperatorPrefixDec : Token::Type::OperatorSuffixDec);
                    else if(isUnaryOperator())
                        addOperatorToken(ch, Token::Type::OperatorUnarySub);
                    else
                        addOperatorToken(ch, Token::Type::OperatorSub);
                    break;
                case '=':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("==", Token::Type::OperatorEqual);
                    else
                        addOperatorToken(ch, Token::Type::OperatorAssign);
                    break;
                case '<':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("<=", Token::Type::OperatorSmallerEqual);
                    else if(tryEatNextCh(i, '<'))
                    {
                        if(tryEatNextCh(i, '='))
                            addOperatorToken("<<=", Token::Type::OperatorAssignShl);
                        else
                            addOperatorToken("<<", Token::Type::OperatorShl);
                    }
                    else
                        addOperatorToken(ch, Token::Type::OperatorSmaller);
                    break;
                case '>':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken(">=", Token::Type::OperatorBiggerEqual);
                    else if(tryEatNextCh(i, '>'))
                    {
                        if(tryEatNextCh(i, '='))
                            addOperatorToken(">>=", Token::Type::OperatorAssignShr);
                        else
                            addOperatorToken(">>", Token::Type::OperatorShr);
                    }
                    else
                        addOperatorToken(ch, Token::Type::OperatorBigger);
                    break;
                case '&':
                    if(tryEatNextCh(i, '&'))
                        addOperatorToken("&&", Token::Type::OperatorLogicalAnd);
                    else if(tryEatNextCh(i, '='))
                        addOperatorToken("&=", Token::Type::OperatorAssignAnd);
                    else
                        addOperatorToken(ch, Token::Type::OperatorAnd);
                    break;
                case '^':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("^=", Token::Type::OperatorAssignXor);
                    else
                        addOperatorToken(ch, Token::Type::OperatorXor);
                    break;
                case '|':
                    if(tryEatNextCh(i, '|'))
                        addOperatorToken("||", Token::Type::OperatorLogicalOr);
                    else if(tryEatNextCh(i, '='))
                        addOperatorToken("|=", Token::Type::OperatorAssignOr);
                    else
                        addOperatorToken(ch, Token::Type::OperatorOr);
                    break;
                case '!':
                    if(tryEatNextCh(i, '='))
                        addOperatorToken("!=", Token::Type::OperatorNotEqual);
                    else
                        addOperatorToken(ch, Token::Type::OperatorLogicalNot);
                    break;
                case ' ': //ignore spaces
                case '\t': //ignore tabs
                    break;
                default:
                    mCurToken += ch;
                    break;
                }
            }
        }
        break;
        }
    }
    if(mCurToken.length() != 0) //make sure the last token is added
        mTokens.push_back(Token(mCurToken, Token::Type::Data));
}

void ExpressionParser::addOperatorToken(const String & data, Token::Type type)
{
    if(mCurToken.length()) //add a new data token when there is data in the buffer
    {
        mTokens.push_back(Token(mCurToken, Token::Type::Data));
        mCurToken.clear();
    }
    mTokens.push_back(Token(data, type)); //add the operator token
}

bool ExpressionParser::isUnaryOperator() const
{
    if(mCurToken.length()) //data before the operator means it is no unary operator
        return false;
    if(!mTokens.size()) //no tokens before the operator means it is an unary operator
        return true;
    auto lastToken = mTokens[mTokens.size() - 1];
    return lastToken.isOperator(); //if the previous operator is a token, the operator is an unary operator
}

void ExpressionParser::shuntingYard()
{
    //Implementation of Dijkstra's Shunting-yard algorithm
    std::vector<Token> queue;
    std::stack<Token> stack;
    auto len = mTokens.size();
    //process the tokens
    for(size_t i = 0; i < len; i++)
    {
        auto & token = mTokens[i];
        switch(token.type())
        {
        case Token::Type::Data:
            queue.push_back(token);
            break;
        case Token::Type::OpenBracket:
            stack.push(token);
            break;
        case Token::Type::CloseBracket:
            while(true)
            {
                if(stack.empty())   //empty stack = bracket mismatch
                {
                    mIsValidExpression = false;
                    return;
                }
                auto curToken = stack.top();
                stack.pop();
                if(curToken.type() == Token::Type::OpenBracket)
                    break;
                queue.push_back(curToken);
            }
            break;
        default: //operator
            auto & o1 = token;
            while(!stack.empty())
            {
                auto o2 = stack.top();
                if(o2.isOperator() &&
                        (o1.associativity() == Token::Associativity::LeftToRight && o1.precedence() >= o2.precedence()) ||
                        (o1.associativity() == Token::Associativity::RightToLeft && o1.precedence() > o2.precedence()))
                {
                    queue.push_back(o2);
                    stack.pop();
                }
                else
                    break;
            }
            stack.push(o1);
            break;
        }
    }
    //pop the remaining operators
    while(!stack.empty())
    {
        auto curToken = stack.top();
        stack.pop();
        if(curToken.type() == Token::Type::OpenBracket || curToken.type() == Token::Type::CloseBracket)  //brackets on the stack means invalid expression
        {
            mIsValidExpression = false;
            return;
        }
        queue.push_back(curToken);
    }
    mPrefixTokens = queue;
}

#ifdef _WIN64
#include <intrin.h>

static unsigned long long umulhi(unsigned long long x, unsigned long long y)
{
    unsigned __int64 res;
    _umul128(x, y, &res);
    return res;
}

static long long mulhi(long long x, long long y)
{
    __int64 res;
    _mul128(x, y, &res);
    return res;
}
#else
static unsigned int umulhi(unsigned int x, unsigned int y)
{
    return (unsigned int)(((unsigned long long)x * y) >> 32);
}

static int mulhi(int x, int y)
{
    return (int)(((long long)x * y) >> 32);
}
#endif //_WIN64

template<typename T>
static bool operation(const ExpressionParser::Token::Type type, const T op1, const T op2, T & result, const bool signedcalc)
{
    result = 0;
    switch(type)
    {
    case ExpressionParser::Token::Type::OperatorUnarySub:
        result = op1 * ~0;
        break;
    case ExpressionParser::Token::Type::OperatorNot:
        result = ~op1;
        break;
    case ExpressionParser::Token::Type::OperatorLogicalNot:
        result = !op1 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorMul:
        result = op1 * op2;
        break;
    case ExpressionParser::Token::Type::OperatorHiMul:
        if(signedcalc)
            result = mulhi(op1, op2);
        else
            result = umulhi(op1, op2);
        break;
    case ExpressionParser::Token::Type::OperatorDiv:
        if(op2 == 0)
            return false;
        result = op1 / op2;
        break;
    case ExpressionParser::Token::Type::OperatorMod:
        if(op2 == 0)
            return false;
        result = op1 % op2;
        break;
    case ExpressionParser::Token::Type::OperatorAdd:
        result = op1 + op2;
        break;
    case ExpressionParser::Token::Type::OperatorSub:
        result = op1 - op2;
        break;
    case ExpressionParser::Token::Type::OperatorShl:
        result = op1 << op2;
        break;
    case ExpressionParser::Token::Type::OperatorShr:
        result = op1 >> op2;
        break;
    case ExpressionParser::Token::Type::OperatorAnd:
        result = op1 & op2;
        break;
    case ExpressionParser::Token::Type::OperatorXor:
        result = op1 ^ op2;
        break;
    case ExpressionParser::Token::Type::OperatorOr:
        result = op1 | op2;
        break;
    case ExpressionParser::Token::Type::OperatorEqual:
        result = op1 == op2 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorNotEqual:
        result = op1 != op2 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorBigger:
        result = op1 > op2 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorSmaller:
        result = op1 < op2 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorBiggerEqual:
        result = op1 >= op2 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorSmallerEqual:
        result = op1 <= op2 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorLogicalAnd:
        result = op1 && op2 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorLogicalOr:
        result = op1 || op2 ? 1 : 0;
        break;
    case ExpressionParser::Token::Type::OperatorLogicalImpl:
        result = !op1 || op2 ? 1 : 0;
        break;
    default:
        return false;
    }
    return true;
}

static bool getAssignmentOperator(ExpressionParser::Token::Type type, ExpressionParser::Token::Type & result)
{
    switch(type)
    {
    case ExpressionParser::Token::Type::OperatorAssign:
        return false;
    case ExpressionParser::Token::Type::OperatorAssignMul:
        result = ExpressionParser::Token::Type::OperatorMul;
        break;
    case ExpressionParser::Token::Type::OperatorAssignHiMul:
        result = ExpressionParser::Token::Type::OperatorHiMul;
        break;
    case ExpressionParser::Token::Type::OperatorAssignDiv:
        result = ExpressionParser::Token::Type::OperatorDiv;
        break;
    case ExpressionParser::Token::Type::OperatorAssignMod:
        result = ExpressionParser::Token::Type::OperatorMod;
        break;
    case ExpressionParser::Token::Type::OperatorAssignAdd:
        result = ExpressionParser::Token::Type::OperatorAdd;
        break;
    case ExpressionParser::Token::Type::OperatorAssignSub:
        result = ExpressionParser::Token::Type::OperatorSub;
        break;
    case ExpressionParser::Token::Type::OperatorAssignShl:
        result = ExpressionParser::Token::Type::OperatorShl;
        break;
    case ExpressionParser::Token::Type::OperatorAssignShr:
        result = ExpressionParser::Token::Type::OperatorShr;
        break;
    case ExpressionParser::Token::Type::OperatorAssignAnd:
        result = ExpressionParser::Token::Type::OperatorAnd;
        break;
    case ExpressionParser::Token::Type::OperatorAssignXor:
        result = ExpressionParser::Token::Type::OperatorXor;
        break;
    case ExpressionParser::Token::Type::OperatorAssignOr:
        result = ExpressionParser::Token::Type::OperatorOr;
        break;
    default:
        __debugbreak();
    }
    return true;
}

static bool handleAssignment(const char* variable, duint resultv, bool silent, bool allowassign)
{
    if(!allowassign)
        return false;
    bool destIsVar = false;
    duint temp;
    //TODO: implement destIsVar without retrieving the value
    valfromstring_noexpr(variable, &temp, true, false, nullptr, &destIsVar, nullptr); //there is no return check on this because the destination might not exist yet
    if(!destIsVar)
        destIsVar = vargettype(variable, nullptr);
    if(!destIsVar || !valtostring(variable, resultv, true))
    {
        duint value;
        if(valfromstring(variable, &value))    //if the var is a value already it's an invalid destination
        {
            if(!silent)
                dprintf("invalid dest \"%s\"\n", variable);
            return false;
        }
        varnew(variable, resultv, VAR_USER);
    }
    return true;
}

template<typename T>
static bool evalOperation(ExpressionParser::Token::Type type, const ExpressionParser::EvalValue & op1, const ExpressionParser::EvalValue & op2, ExpressionParser::EvalValue & result, bool signedcalc, bool silent, bool baseonly, bool allowassign)
{
    switch(type)
    {
    case ExpressionParser::Token::Type::OperatorAssign:
    case ExpressionParser::Token::Type::OperatorAssignMul:
    case ExpressionParser::Token::Type::OperatorAssignHiMul:
    case ExpressionParser::Token::Type::OperatorAssignDiv:
    case ExpressionParser::Token::Type::OperatorAssignMod:
    case ExpressionParser::Token::Type::OperatorAssignAdd:
    case ExpressionParser::Token::Type::OperatorAssignSub:
    case ExpressionParser::Token::Type::OperatorAssignShl:
    case ExpressionParser::Token::Type::OperatorAssignShr:
    case ExpressionParser::Token::Type::OperatorAssignAnd:
    case ExpressionParser::Token::Type::OperatorAssignXor:
    case ExpressionParser::Token::Type::OperatorAssignOr:
    {
        if(op1.evaluated)
            return false;
        ExpressionParser::EvalValue newvalue(0);
        ExpressionParser::Token::Type assop;
        if(getAssignmentOperator(type, assop))
        {
            if(!evalOperation<T>(assop, op1, op2, newvalue, signedcalc, silent, baseonly, allowassign))
                return false;
        }
        else
            newvalue = op2;
        duint resultv;
        if(!newvalue.DoEvaluate(resultv, silent, baseonly))
            return false;
        if(!handleAssignment(op1.data.c_str(), resultv, silent, allowassign))
            return false;
        result = ExpressionParser::EvalValue(resultv);
    }
    break;

    case ExpressionParser::Token::Type::OperatorPrefixInc:
    case ExpressionParser::Token::Type::OperatorPrefixDec:
    case ExpressionParser::Token::Type::OperatorSuffixInc:
    case ExpressionParser::Token::Type::OperatorSuffixDec:
    {
        if(op1.evaluated)
            return false;
        duint op1v;
        if(!op1.DoEvaluate(op1v, silent, baseonly))
            return false;
        duint resultv;
        switch(type)
        {
        case ExpressionParser::Token::Type::OperatorPrefixInc:
            resultv = ++op1v;
            break;
        case ExpressionParser::Token::Type::OperatorPrefixDec:
            resultv = --op1v;
            break;
        case ExpressionParser::Token::Type::OperatorSuffixInc:
            resultv = op1v++;
            break;
        case ExpressionParser::Token::Type::OperatorSuffixDec:
            resultv = op1v--;
            break;
        default:
            return false;
        }
        if(!handleAssignment(op1.data.c_str(), op1v, silent, allowassign))
            return false;
        result = ExpressionParser::EvalValue(resultv);
    }
    break;

    default:
    {
        duint op1v, op2v;
        if(!op1.DoEvaluate(op1v, silent, baseonly) || !op2.DoEvaluate(op2v, silent, baseonly))
            return false;
        T resultv;
        if(!operation<T>(type, T(op1v), T(op2v), resultv, signedcalc))
            return false;
        result = ExpressionParser::EvalValue(duint(resultv));
    }
    break;
    }
    return true;
}

bool ExpressionParser::unsignedOperation(Token::Type type, const EvalValue & op1, const EvalValue & op2, EvalValue & result, bool silent, bool baseonly, bool allowassign) const
{
    return evalOperation<duint>(type, op1, op2, result, false, silent, baseonly, allowassign);
}

bool ExpressionParser::signedOperation(Token::Type type, const EvalValue & op1, const EvalValue & op2, EvalValue & result, bool silent, bool baseonly, bool allowassign) const
{
    return evalOperation<dsint>(type, op1, op2, result, true, silent, baseonly, allowassign);
}

bool ExpressionParser::Calculate(duint & value, bool signedcalc, bool allowassign, bool silent, bool baseonly, int* value_size, bool* isvar, bool* hexonly) const
{
    value = 0;
    if(!mPrefixTokens.size() || !mIsValidExpression)
        return false;
    std::stack<EvalValue> stack;
    //calculate the result from the RPN queue
    for(const auto & token : mPrefixTokens)
    {
        if(token.isOperator())
        {
            EvalValue op1(0);
            EvalValue op2(0);
            EvalValue result(0);
            bool operationSuccess;
            auto type = token.type();
            switch(type)
            {
            case Token::Type::OperatorUnarySub:
            case Token::Type::OperatorNot:
            case Token::Type::OperatorLogicalNot:
            case Token::Type::OperatorPrefixInc:
            case Token::Type::OperatorPrefixDec:
            case Token::Type::OperatorSuffixInc:
            case Token::Type::OperatorSuffixDec:
                if(stack.size() < 1)
                    return false;
                op1 = stack.top();
                stack.pop();
                if(signedcalc)
                    operationSuccess = signedOperation(type, op1, op2, result, silent, baseonly, allowassign);
                else
                    operationSuccess = unsignedOperation(type, op1, op2, result, silent, baseonly, allowassign);
                if(!operationSuccess)
                    return false;
                stack.push(result);
                break;
            case Token::Type::OperatorMul:
            case Token::Type::OperatorHiMul:
            case Token::Type::OperatorDiv:
            case Token::Type::OperatorMod:
            case Token::Type::OperatorAdd:
            case Token::Type::OperatorSub:
            case Token::Type::OperatorShl:
            case Token::Type::OperatorShr:
            case Token::Type::OperatorAnd:
            case Token::Type::OperatorXor:
            case Token::Type::OperatorOr:
            case Token::Type::OperatorEqual:
            case Token::Type::OperatorNotEqual:
            case Token::Type::OperatorBigger:
            case Token::Type::OperatorSmaller:
            case Token::Type::OperatorBiggerEqual:
            case Token::Type::OperatorSmallerEqual:
            case Token::Type::OperatorLogicalAnd:
            case Token::Type::OperatorLogicalOr:
            case Token::Type::OperatorLogicalImpl:
            case Token::Type::OperatorAssign:
            case Token::Type::OperatorAssignMul:
            case Token::Type::OperatorAssignHiMul:
            case Token::Type::OperatorAssignDiv:
            case Token::Type::OperatorAssignMod:
            case Token::Type::OperatorAssignAdd:
            case Token::Type::OperatorAssignSub:
            case Token::Type::OperatorAssignShl:
            case Token::Type::OperatorAssignShr:
            case Token::Type::OperatorAssignAnd:
            case Token::Type::OperatorAssignXor:
            case Token::Type::OperatorAssignOr:
                if(stack.size() < 2)
                    return false;
                op2 = stack.top();
                stack.pop();
                op1 = stack.top();
                stack.pop();
                if(signedcalc)
                    operationSuccess = signedOperation(type, op1, op2, result, silent, baseonly, allowassign);
                else
                    operationSuccess = unsignedOperation(type, op1, op2, result, silent, baseonly, allowassign);
                if(!operationSuccess)
                    return false;
                stack.push(result);
                break;
            case Token::Type::Error:
                return false;
            default: //do nothing
                break;
            }
        }
        else
        {
            stack.push(EvalValue(token.data()));
        }
    }
    if(stack.empty())  //empty result stack means error
        return false;
    return stack.top().DoEvaluate(value, silent, baseonly, value_size, isvar, hexonly);
}
