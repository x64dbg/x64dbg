#include "expressionparser.h"
#include "value.h"
#include "console.h"
#include "variable.h"
#include "expressionfunctions.h"
#include <algorithm>

ExpressionParser::Token::Associativity ExpressionParser::Token::associativity() const
{
    switch(mType)
    {
    case Type::OperatorUnarySub:
    case Type::OperatorUnaryAdd:
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
    case Type::OperatorAssignRol:
    case Type::OperatorAssignRor:
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
    case Type::OperatorRol:
    case Type::OperatorRor:
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
    case Type::OperatorUnaryAdd:
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
    case Type::OperatorRol:
    case Type::OperatorRor:
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
    case Type::OperatorAssignRol:
    case Type::OperatorAssignRor:
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
    return mType >= Type::OperatorUnarySub;
}

ExpressionParser::ExpressionParser(const String & expression)
    : mExpression(fixClosingBrackets(expression)),
      mIsValidExpression(true)
{
    const size_t r = 50;
    mTokens.reserve(r);
    mCurToken.reserve(r);
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
    auto stateQuote = false;
    auto len = mExpression.length();
    auto push_token = [this, &stateQuote](char ch)
    {
        mCurToken.push_back(ch);
        mCurTokenQuoted.push_back(stateQuote);
    };
    for(size_t i = 0; i < len; i++)
    {
        auto ch = mExpression[i];
        switch(ch)
        {
        case '\"':
        {
            stateQuote = !stateQuote;
        }
        break;

        case '[':
        {
            stateMemory++;
            push_token(ch);
        }
        break;

        case ']':
        {
            if(stateMemory)
                stateMemory--;
            else
                mIsValidExpression = false;
            push_token(ch);
        }
        break;

        default:
        {
            if(stateMemory || stateQuote)
                push_token(ch);
            else
            {
                switch(ch)
                {
                case ',':
                    addOperatorToken(ch, Token::Type::Comma);
                    break;
                case '(':
                    addOperatorToken(ch, Token::Type::OpenParen);
                    break;
                case ')':
                    addOperatorToken(ch, Token::Type::CloseParen);
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
                    else if(isUnaryOperator())
                        addOperatorToken(ch, Token::Type::OperatorUnaryAdd);
                    else
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
                        if(tryEatNextCh(i, '<'))
                        {
                            if(tryEatNextCh(i, '='))
                                addOperatorToken("<<<=", Token::Type::OperatorAssignRol);
                            else
                                addOperatorToken("<<<", Token::Type::OperatorRol);
                        }
                        else if(tryEatNextCh(i, '='))
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
                        if(tryEatNextCh(i, '>'))
                        {
                            if(tryEatNextCh(i, '='))
                                addOperatorToken(">>>=", Token::Type::OperatorAssignRor);
                            else
                                addOperatorToken(">>>", Token::Type::OperatorRor);
                        }
                        else if(tryEatNextCh(i, '='))
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
                    push_token(ch);
                    break;
                }
            }
        }
        break;
        }
    }
    if(mCurToken.length() != 0) //make sure the last token is added
    {
        mTokens.push_back(Token(mCurToken, resolveQuotedData()));
        mCurToken.clear();
        mCurTokenQuoted.clear();
    }
}

void ExpressionParser::addOperatorToken(const String & data, Token::Type type)
{
    if(mCurToken.length()) //add a new data token when there is data in the buffer
    {
        if(type == Token::Type::OpenParen)
        {
            mTokens.push_back(Token(mCurToken, Token::Type::Function));
        }
        else
        {
            mTokens.push_back(Token(mCurToken, resolveQuotedData()));
        }
        mCurToken.clear();
        mCurTokenQuoted.clear();
    }
    mTokens.push_back(Token(data, type)); //add the operator token
}

ExpressionParser::Token::Type ExpressionParser::resolveQuotedData() const
{
    auto allQuoted = std::find(mCurTokenQuoted.begin(), mCurTokenQuoted.end(), false) == mCurTokenQuoted.end();
    return allQuoted ? Token::Type::QuotedData : Token::Type::Data;
}

bool ExpressionParser::isUnaryOperator() const
{
    if(mCurToken.length()) //data before the operator means it is no unary operator
        return false;
    if(!mTokens.size()) //no tokens before the operator means it is an unary operator
        return true;
    auto lastType = mTokens.back().type();
    //if the previous token is not data or a close bracket, this operator is a unary operator
    return lastType != Token::Type::Data && lastType != Token::Type::QuotedData && lastType != Token::Type::CloseParen;
}

void ExpressionParser::shuntingYard()
{
    //Implementation of Dijkstra's Shunting-yard algorithm (https://en.wikipedia.org/wiki/Shunting-yard_algorithm)
    std::vector<Token> queue;
    std::vector<Token> stack;
    std::vector<duint> argCount;
    auto len = mTokens.size();
    queue.reserve(len);
    stack.reserve(len);
    //process the tokens
    for(size_t i = 0; i < len; i++)
    {
        const auto & token = mTokens[i]; //Read a token
        switch(token.type())
        {
        case Token::Type::Data: //If the token is a number, then push it to the output queue.
        case Token::Type::QuotedData:
            queue.push_back(token);
            break;
        case Token::Type::Function: //If the token is a function token, then push it onto the stack.
        {
            stack.push_back(token);

            // Unless the syntax is 'fn()' there is always at least one argument
            if(i + 2 < mTokens.size() && mTokens[i + 1].type() == Token::Type::OpenParen && mTokens[i + 2].type() == Token::Type::CloseParen)
                argCount.push_back(0);
            else
                argCount.push_back(1);
        }
        break;
        case Token::Type::Comma: //If the token is a function argument separator (e.g., a comma):
        {
            while(true) //Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
            {
                if(stack.empty()) //If no left parentheses are encountered, either the separator was misplaced or parentheses were mismatched.
                {
                    mIsValidExpression = false;
                    return;
                }
                const auto & curToken = stack.back();
                if(curToken.type() == Token::Type::OpenParen)
                    break;
                queue.push_back(curToken);
                stack.pop_back();
            }

            if(!argCount.empty()) // A comma increases the argument count
                argCount.back()++;
        }
        break;
        case Token::Type::OpenParen: //If the token is a left parenthesis (i.e. "("), then push it onto the stack.
            stack.push_back(token);
            break;
        case Token::Type::CloseParen: //If the token is a right parenthesis (i.e. ")"):
        {
            while(true) //Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
            {
                if(stack.empty()) //If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
                {
                    mIsValidExpression = false;
                    return;
                }
                auto curToken = stack.back();
                stack.pop_back(); //Pop the left parenthesis from the stack, but not onto the output queue.
                if(curToken.type() == Token::Type::OpenParen) //the bracket is already popped here
                    break;
                queue.push_back(curToken);
            }
            if(!stack.empty() && stack.back().type() == Token::Type::Function) //If the token at the top of the stack is a function token, pop it onto the output queue.
            {
                // Propagate the argument count as extra information
                stack.back().setInfo(argCount.back());
                argCount.pop_back();
                queue.push_back(stack.back());
                stack.pop_back();
            }
        }
        break;
        default: //If the token is an operator, o1, then:
            const auto & o1 = token;
            while(!stack.empty()) //while there is an operator token o2, at the top of the operator stack and either
            {
                const auto & o2 = stack.back();
                if(o2.isOperator() &&
                        (o1.associativity() == Token::Associativity::LeftToRight && o1.precedence() >= o2.precedence()) || //o1 is left-associative and its precedence is less than or equal to that of o2, or
                        (o1.associativity() == Token::Associativity::RightToLeft && o1.precedence() > o2.precedence())) //o1 is right associative, and has precedence less than that of o2,
                {
                    queue.push_back(o2); //pop o2 off the operator stack, onto the output queue;
                    stack.pop_back();
                }
                else
                    break;
            }
            stack.push_back(o1); //at the end of iteration push o1 onto the operator stack.
            break;
        }
    }
    //When there are no more tokens to read:
    while(!stack.empty()) //While there are still operator tokens in the stack:
    {
        const auto & curToken = stack.back();
        if(curToken.type() == Token::Type::OpenParen || curToken.type() == Token::Type::CloseParen) //If the operator token on the top of the stack is a parenthesis, then there are mismatched parentheses.
        {
            mIsValidExpression = false;
            return;
        }
        queue.push_back(curToken); //Pop the operator onto the output queue.
        stack.pop_back();
    }
    mPrefixTokens = std::move(queue);
}

#if defined(_WIN64) && (!defined(__clang__) || __clang_major__ > 3) // This produces an ICE under Clang <= 3.8; fixed in 5.0.
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
static bool operation(ExpressionParser::Token::Type type, T op1, T op2, T & result, bool signedcalc)
{
    result = 0;
    switch(type)
    {
    case ExpressionParser::Token::Type::OperatorUnarySub:
        result = op1 * ~0;
        break;
    case ExpressionParser::Token::Type::OperatorUnaryAdd:
        result = +op1;
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
    case ExpressionParser::Token::Type::OperatorRol:
#ifdef _WIN64
        result = _rotl64(op1, int(op2) % 64);
#else
        result = _rotl(op1, int(op2) % 32);
#endif
        break;
    case ExpressionParser::Token::Type::OperatorRor:
#ifdef _WIN64
        result = _rotr64(op1, int(op2) % 64);
#else
        result = _rotr(op1, int(op2) % 32);
#endif
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
    case ExpressionParser::Token::Type::OperatorAssignRol:
        result = ExpressionParser::Token::Type::OperatorRol;
        break;
    case ExpressionParser::Token::Type::OperatorAssignRor:
        result = ExpressionParser::Token::Type::OperatorRor;
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
    valfromstring_noexpr(variable, &temp, true, true, nullptr, &destIsVar, nullptr); //there is no return check on this because the destination might not exist yet
    if(!destIsVar)
        destIsVar = vargettype(variable, nullptr);
    if(!destIsVar || !valtostring(variable, resultv, true))
    {
        duint value;
        if(valfromstring(variable, &value)) //if the var is a value already it's an invalid destination
        {
            if(!silent)
                dprintf(QT_TRANSLATE_NOOP("DBG", "invalid dest \"%s\"\n"), variable);
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
    case ExpressionParser::Token::Type::OperatorAssignRol:
    case ExpressionParser::Token::Type::OperatorAssignRor:
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
        if(op1.isString || op2.isString)
            return false;
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
    EvalValue evalue(0);
    if(!Calculate(evalue, signedcalc, allowassign, silent, baseonly, value_size, isvar, hexonly))
        return false;

    if(evalue.isString)
    {
        if(!silent)
            dprintf(QT_TRANSLATE_NOOP("DBG", "Expression evaluated to a string: \"%s\"\n"), StringUtils::Escape(evalue.data).c_str());
        return false;
    }
    return evalue.DoEvaluate(value, silent, baseonly, value_size, isvar, hexonly);
}

bool ExpressionParser::Calculate(EvalValue & value, bool signedcalc, bool allowassign, bool silent, bool baseonly, int* value_size, bool* isvar, bool* hexonly) const
{
    if(!mPrefixTokens.size() || !mIsValidExpression)
        return false;
    std::vector<EvalValue> stack;
    stack.reserve(mPrefixTokens.size());
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
            case Token::Type::OperatorUnaryAdd:
            case Token::Type::OperatorNot:
            case Token::Type::OperatorLogicalNot:
            case Token::Type::OperatorPrefixInc:
            case Token::Type::OperatorPrefixDec:
            case Token::Type::OperatorSuffixInc:
            case Token::Type::OperatorSuffixDec:
                if(stack.empty())
                    return false;
                op1 = stack.back();
                stack.pop_back();
                if(signedcalc)
                    operationSuccess = signedOperation(type, op1, op2, result, silent, baseonly, allowassign);
                else
                    operationSuccess = unsignedOperation(type, op1, op2, result, silent, baseonly, allowassign);
                if(!operationSuccess)
                    return false;
                stack.push_back(result);
                break;
            case Token::Type::OperatorMul:
            case Token::Type::OperatorHiMul:
            case Token::Type::OperatorDiv:
            case Token::Type::OperatorMod:
            case Token::Type::OperatorAdd:
            case Token::Type::OperatorSub:
            case Token::Type::OperatorShl:
            case Token::Type::OperatorShr:
            case Token::Type::OperatorRol:
            case Token::Type::OperatorRor:
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
            case Token::Type::OperatorAssignRol:
            case Token::Type::OperatorAssignRor:
            case Token::Type::OperatorAssignAnd:
            case Token::Type::OperatorAssignXor:
            case Token::Type::OperatorAssignOr:
                if(stack.size() < 2)
                    return false;
                op2 = stack.back();
                stack.pop_back();
                op1 = stack.back();
                stack.pop_back();
                if(signedcalc)
                    operationSuccess = signedOperation(type, op1, op2, result, silent, baseonly, allowassign);
                else
                    operationSuccess = unsignedOperation(type, op1, op2, result, silent, baseonly, allowassign);
                if(!operationSuccess)
                    return false;
                stack.push_back(result);
                break;
            case Token::Type::Error:
                return false;
            default: //do nothing
                break;
            }
        }
        else if(token.type() == Token::Type::Function)
        {
            const auto & name = token.data();
            ValueType returnType;
            std::vector<ValueType> argTypes;
            if(!ExpressionFunctions::GetType(name, returnType, argTypes))
                return false;
            if(int(stack.size()) < argTypes.size())
                return false;
            std::vector<ExpressionValue> argv;
            argv.resize(argTypes.size());
            for(size_t i = 0; i < argTypes.size(); i++)
            {
                const auto & argType = argTypes[argTypes.size() - i - 1];
                auto & top = stack[stack.size() - i - 1];
                ExpressionValue arg;
                if(top.isString)
                {
                    arg = { ValueTypeString, 0, StringValue{ top.data.c_str(), false } };
                }
                else if(top.evaluated)
                {
                    arg = { ValueTypeNumber, top.value };
                }
                else
                {
                    duint result;
                    if(!top.DoEvaluate(result, silent, baseonly, value_size, isvar, hexonly))
                        return false;
                    arg = { ValueTypeNumber, result };
                }

                if(arg.type != argType && argType != ValueTypeAny)
                {
                    if(!silent)
                    {
                        auto typeName = [](ValueType t) -> String
                        {
                            switch(t)
                            {
                            case ValueTypeNumber:
                                return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "number"));
                            case ValueTypeString:
                                return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "string"));
                            }
                            return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "invalid"));
                        };
                        String argValueStr;
                        if(arg.type == ValueTypeNumber)
                        {
                            argValueStr = StringUtils::sprintf("0x%p", arg.number);
                        }
                        else if(arg.type == ValueTypeString)
                        {
                            argValueStr = "\"" + StringUtils::Escape(arg.string.ptr) + "\"";
                        }
                        else
                        {
                            argValueStr = "???";
                        }
                        String signature = name;
                        signature += "(";
                        for(size_t j = 0; j < argTypes.size(); j++)
                        {
                            if(j > 0)
                            {
                                signature += ", ";
                            }
                            signature += typeName(argTypes[j]);
                        }
                        signature += ")";
                        dprintf(QT_TRANSLATE_NOOP("DBG", "Expression function %s argument %d/%d (%s) type mismatch (expected %s, got %s)!\n"),
                                signature.c_str(),
                                argTypes.size() - i,
                                argTypes.size(),
                                argValueStr.c_str(),
                                typeName(argType).c_str(),
                                typeName(arg.type).c_str()
                               );
                    }
                    return false;
                }

                argv[argTypes.size() - i - 1] = arg;
            }

            ExpressionValue result = { ValueTypeNumber, 0 };
            if(!ExpressionFunctions::Call(name, result, argv))
                return false;

            if(result.type == ValueTypeAny)
                return false;

            for(size_t i = 0; i < argv.size(); i++)
            {
                stack.pop_back();
            }
            if(result.type == ValueTypeString)
            {
                stack.push_back(EvalValue(result.string.ptr, true));
                if(result.string.isOwner)
                    BridgeFree((void*)result.string.ptr);
            }
            else
                stack.push_back(EvalValue(result.number));
        }
        else
            stack.push_back(EvalValue(token.data(), token.type() == Token::Type::QuotedData));
    }
    if(stack.size() != 1) //there should only be one value left on the stack
        return false;
    value = stack.back();
    return true;
}
