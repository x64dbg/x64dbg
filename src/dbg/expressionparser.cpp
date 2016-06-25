#include "expressionparser.h"
#include "value.h"

ExpressionParser::Token::Token(const String & data, const Type type)
{
    mData = data;
    mType = type;
}

const String & ExpressionParser::Token::data() const
{
    return mData;
}

ExpressionParser::Token::Type ExpressionParser::Token::type() const
{
    return mType;
}

ExpressionParser::Token::Associativity ExpressionParser::Token::associativity() const
{
    switch(mType)
    {
    case Type::OperatorUnarySub:
    case Type::OperatorNot:
    case Type::OperatorLogicalNot:
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
    case Type::OperatorUnarySub:
    case Type::OperatorNot:
    case Type::OperatorLogicalNot:
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
        return 12;
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
                    addOperatorToken(ch, Token::Type::OperatorMul);
                    break;
                case '`':
                    addOperatorToken(ch, Token::Type::OperatorHiMul);
                    break;
                case '/':
                    addOperatorToken(ch, Token::Type::OperatorDiv);
                    break;
                case '%':
                    addOperatorToken(ch, Token::Type::OperatorMod);
                    break;
                case '+':
                    if(!isUnaryOperator())    //skip all unary add operators
                        addOperatorToken(ch, Token::Type::OperatorAdd);
                    break;
                case '-':
                    if(isUnaryOperator())
                        addOperatorToken(ch, Token::Type::OperatorUnarySub);
                    else
                        addOperatorToken(ch, Token::Type::OperatorSub);
                    break;
                case '=':
                    if(i + 1 < len && mExpression[i + 1] == '=')
                    {
                        addOperatorToken(ch, Token::Type::OperatorEqual);
                        i++;
                    }
                    else
                    {
                        addOperatorToken(ch, Token::Type::Error);
                        mIsValidExpression = false;
                    }
                    break;
                case '<':
                    if(i + 1 < len && mExpression[i + 1] == '=')
                    {
                        addOperatorToken(ch, Token::Type::OperatorSmallerEqual);
                        i++;
                    }
                    else if(i + 1 < len && mExpression[i + 1] == '<')
                    {
                        addOperatorToken(ch, Token::Type::OperatorShl);
                        i++;
                    }
                    else
                        addOperatorToken(ch, Token::Type::OperatorSmaller);
                    break;
                case '>':
                    if(i + 1 < len && mExpression[i + 1] == '=')
                    {
                        addOperatorToken(ch, Token::Type::OperatorBiggerEqual);
                        i++;
                    }
                    else if(i + 1 < len && mExpression[i + 1] == '>')
                    {
                        addOperatorToken(ch, Token::Type::OperatorShr);
                        i++;
                    }
                    else
                        addOperatorToken(ch, Token::Type::OperatorBigger);
                    break;
                case '&':
                    if(i + 1 < len && mExpression[i + 1] == '&')
                    {
                        addOperatorToken(ch, Token::Type::OperatorLogicalAnd);
                        i++;
                    }
                    else
                        addOperatorToken(ch, Token::Type::OperatorAnd);
                    break;
                case '^':
                    addOperatorToken(ch, Token::Type::OperatorXor);
                    break;
                case '|':
                    if(i + 1 < len && mExpression[i + 1] == '|')
                    {
                        addOperatorToken(ch, Token::Type::OperatorLogicalOr);
                        i++;
                    }
                    else
                        addOperatorToken(ch, Token::Type::OperatorOr);
                    break;
                case '!':
                    if(i + 1 < len && mExpression[i + 1] == '=')
                    {
                        addOperatorToken(ch, Token::Type::OperatorNotEqual);
                        i++;
                    }
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

void ExpressionParser::addOperatorToken(const char ch, const Token::Type type)
{
    if(mCurToken.length()) //add a new data token when there is data in the buffer
    {
        mTokens.push_back(Token(mCurToken, Token::Type::Data));
        mCurToken.clear();
    }
    String data;
    data += ch;
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
    default:
        return false;
    }
    return true;
}

bool ExpressionParser::unsignedOperation(const Token::Type type, const duint op1, const duint op2, duint & result) const
{
    return operation<duint>(type, op1, op2, result, false);
}

bool ExpressionParser::signedOperation(const Token::Type type, const dsint op1, const dsint op2, duint & result) const
{
    dsint signedResult;
    if(!operation<dsint>(type, op1, op2, signedResult, true))
        return false;
    result = duint(signedResult);
    return true;
}

bool ExpressionParser::Calculate(duint & value, bool signedcalc, bool silent, bool baseonly, int* value_size, bool* isvar, bool* hexonly) const
{
    value = 0;
    if(!mPrefixTokens.size() || !mIsValidExpression)
        return false;
    std::stack<duint> stack;
    //calculate the result from the RPN queue
    for(const auto & token : mPrefixTokens)
    {
        if(token.isOperator())
        {
            duint op1 = 0;
            duint op2 = 0;
            duint result = 0;
            switch(token.type())
            {
            case Token::Type::OperatorUnarySub:
            case Token::Type::OperatorNot:
            case Token::Type::OperatorLogicalNot:
                if(stack.size() < 1)
                    return false;
                op1 = stack.top();
                stack.pop();
                if(signedcalc)
                    signedOperation(token.type(), op1, op2, result);
                else
                    unsignedOperation(token.type(), op1, op2, result);
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
                if(stack.size() < 2)
                    return false;
                op2 = stack.top();
                stack.pop();
                op1 = stack.top();
                stack.pop();
                if(signedcalc)
                    signedOperation(token.type(), op1, op2, result);
                else
                    unsignedOperation(token.type(), op1, op2, result);
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
            duint result;
            if(!valfromstring_noexpr(token.data().c_str(), &result, silent, baseonly, value_size, isvar, hexonly))
                return false;
            stack.push(result);
        }
    }
    if(stack.empty())  //empty result stack means error
        return false;
    value = stack.top();
    return true;
}
