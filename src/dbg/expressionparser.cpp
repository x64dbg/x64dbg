#include "expressionparser.h"
#include "value.h"

ExpressionParser::Token::Token(const String & data, const Type type)
{
    _data = data;
    _type = type;
}

const String & ExpressionParser::Token::data() const
{
    return _data;
}

ExpressionParser::Token::Type ExpressionParser::Token::type() const
{
    return _type;
}

ExpressionParser::Token::Associativity ExpressionParser::Token::associativity() const
{
    switch(_type)
    {
    case Type::OperatorUnarySub:
    case Type::OperatorNot:
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
        return Associativity::LeftToRight;
    default:
        return Associativity::Unspecified;
    }
}

int ExpressionParser::Token::precedence() const
{
    switch(_type)
    {
    case Type::OperatorUnarySub:
    case Type::OperatorNot:
        return 7;
    case Type::OperatorMul:
    case Type::OperatorHiMul:
    case Type::OperatorDiv:
    case Type::OperatorMod:
        return 6;
    case Type::OperatorAdd:
    case Type::OperatorSub:
        return 5;
    case Type::OperatorShl:
    case Type::OperatorShr:
        return 4;
    case Type::OperatorAnd:
        return 3;
    case Type::OperatorXor:
        return 2;
    case Type::OperatorOr:
        return 1;
    default:
        return 0;
    }
}

bool ExpressionParser::Token::isOperator() const
{
    return _type != Type::Data && _type != Type::OpenBracket && _type != Type::CloseBracket;
}

ExpressionParser::ExpressionParser(const String & expression)
{
    _tokens.clear();
    _prefixTokens.clear();
    tokenize(fixClosingBrackets(expression));
    shuntingYard();
}

String ExpressionParser::fixClosingBrackets(const String & expression)
{
    int open = 0;
    int close = 0;
    size_t len = expression.length();
    for(size_t i = 0; i < len; i++)
    {
        if(expression[i] == '(')
            open++;
        else if(expression[i] == ')')
            close++;
    }
    String result = expression;
    if(close < open)
    {
        for(int i = 0; i < open - close; i++)
            result += ")";
    }
    return result;
}

void ExpressionParser::tokenize(const String & expression)
{
    bool stateMemory = false;
    size_t len = expression.length();
    for(size_t i = 0; i < len; i++)
    {
        char ch = expression[i];
        switch(ch)
        {
        case '[':
        {
            stateMemory = true;
            _curToken += ch;
        }
        break;

        case ']':
        {
            stateMemory = false;
            _curToken += ch;
        }
        break;

        default:
        {
            if(stateMemory)
                _curToken += ch;
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
                    if(!isUnaryOperator())   //skip all unary add operators
                        addOperatorToken(ch, Token::Type::OperatorAdd);
                    break;
                case '-':
                    if(isUnaryOperator())
                        addOperatorToken(ch, Token::Type::OperatorUnarySub);
                    else
                        addOperatorToken(ch, Token::Type::OperatorSub);
                    break;
                case '<':
                    addOperatorToken(ch, Token::Type::OperatorShl);
                    break;
                case '>':
                    addOperatorToken(ch, Token::Type::OperatorShr);
                    break;
                case '&':
                    addOperatorToken(ch, Token::Type::OperatorAnd);
                    break;
                case '^':
                    addOperatorToken(ch, Token::Type::OperatorXor);
                    break;
                case '|':
                    addOperatorToken(ch, Token::Type::OperatorOr);
                    break;
                case ' ': //ignore spaces
                    break;
                default:
                    _curToken += ch;
                    break;
                }
            }
        }
        break;
        }
    }
    if(_curToken.length() != 0)  //make sure the last token is added
        _tokens.push_back(Token(_curToken, Token::Type::Data));
}

void ExpressionParser::addOperatorToken(const char ch, const Token::Type type)
{
    if(_curToken.length())  //add a new data token when there is data in the buffer
    {
        _tokens.push_back(Token(_curToken, Token::Type::Data));
        _curToken.clear();
    }
    String data;
    data += ch;
    _tokens.push_back(Token(data, type)); //add the operator token
}

bool ExpressionParser::isUnaryOperator()
{
    if(_curToken.length())  //data before the operator means it is no unary operator
        return false;
    if(!_tokens.size())  //no tokens before the operator means it is an unary operator
        return true;
    Token lastToken = _tokens[_tokens.size() - 1];
    return lastToken.isOperator(); //if the previous operator is a token, the operator is an unary operator
}

void ExpressionParser::shuntingYard()
{
    //Implementation of Dijkstra's Shunting-yard algorithm
    std::vector<Token> queue;
    std::stack<Token> stack;
    size_t len = _tokens.size();
    //process the tokens
    for(size_t i = 0; i < len; i++)
    {
        Token & token = _tokens[i];
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
                if(stack.empty())  //empty stack = bracket mismatch
                    return;
                Token curToken = stack.top();
                stack.pop();
                if(curToken.type() == Token::Type::OpenBracket)
                    break;
                queue.push_back(curToken);
            }
            break;
        default: //operator
            Token & o1 = token;
            while(!stack.empty())
            {
                Token o2 = stack.top();
                if(o2.isOperator() &&
                        (o1.associativity() == Token::Associativity::LeftToRight && o1.precedence() <= o2.precedence()) ||
                        (o1.associativity() == Token::Associativity::RightToLeft && o1.precedence() < o2.precedence()))
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
        Token curToken = stack.top();
        stack.pop();
        if(curToken.type() == Token::Type::OpenBracket || curToken.type() == Token::Type::CloseBracket)  //brackets on the stack means invalid expression
            return;
        queue.push_back(curToken);
    }
    _prefixTokens = queue;
}

#ifdef _WIN64
#include <intrin.h>

static inline unsigned long long umulhi(unsigned long long x, unsigned long long y)
{
    unsigned __int64 res;
    _umul128(x, y, &res);
    return res;
}

static inline long long mulhi(long long x, long long y)
{
    __int64 res;
    _mul128(x, y, &res);
    return res;
}
#else
static inline unsigned int umulhi(unsigned int x, unsigned int y)
{
    return (unsigned int)(((unsigned long long)x * y) >> 32);
}

static inline int mulhi(int x, int y)
{
    return (int)(((long long)x * y) >> 32);
}
#endif //__MINGW64__

template<typename T>
static bool operation(const ExpressionParser::Token::Type type, const T op1, const T op2, T & result, const bool signedcalc)
{
    result = 0;
    switch(type)
    {
    case ExpressionParser::Token::Type::OperatorUnarySub:
        result = op1 * ~0;
        return true;
    case ExpressionParser::Token::Type::OperatorNot:
        result = ~op1;
        return true;
    case ExpressionParser::Token::Type::OperatorMul:
        result = op1 * op2;
        return true;
    case ExpressionParser::Token::Type::OperatorHiMul:
        if(signedcalc)
            result = mulhi(op1, op2);
        else
            result = umulhi(op1, op2);
        return true;
    case ExpressionParser::Token::Type::OperatorDiv:
        if(op2 != 0)
        {
            result = op1 / op2;
            return true;
        }
        return false;
    case ExpressionParser::Token::Type::OperatorMod:
        if(op2 != 0)
        {
            result = op1 % op2;
            return true;
        }
        return false;
    case ExpressionParser::Token::Type::OperatorAdd:
        result = op1 + op2;
        return true;
    case ExpressionParser::Token::Type::OperatorSub:
        result = op1 - op2;
        return true;
    case ExpressionParser::Token::Type::OperatorShl:
        result = op1 << op2;
        return true;
    case ExpressionParser::Token::Type::OperatorShr:
        result = op1 >> op2;
        return true;
    case ExpressionParser::Token::Type::OperatorAnd:
        result = op1 & op2;
        return true;
    case ExpressionParser::Token::Type::OperatorXor:
        result = op1 ^ op2;
        return true;
    case ExpressionParser::Token::Type::OperatorOr:
        result = op1 | op2;
        return true;
    default:
        return false;
    }
}

bool ExpressionParser::unsignedoperation(const Token::Type type, const uint op1, const uint op2, uint & result)
{
    return operation<uint>(type, op1, op2, result, false);
}

bool ExpressionParser::signedoperation(const Token::Type type, const sint op1, const sint op2, uint & result)
{
    sint signedResult;
    if(!operation<sint>(type, op1, op2, signedResult, true))
        return false;
    result = (uint)signedResult;
    return true;
}

bool ExpressionParser::calculate(uint & value, bool signedcalc, bool silent, bool baseonly, int* value_size, bool* isvar, bool* hexonly)
{
    value = 0;
    if(!_prefixTokens.size())
        return false;
    std::stack<uint> stack;
    //calculate the result from the RPN queue
    for(const auto & token : _prefixTokens)
    {
        if(token.isOperator())
        {
            uint op1 = 0;
            uint op2 = 0;
            uint result = 0;
            switch(token.type())
            {
            case Token::Type::OperatorUnarySub:
            case Token::Type::OperatorNot:
                if(stack.size() < 1)
                    return false;
                op1 = stack.top();
                stack.pop();
                if(signedcalc)
                    signedoperation(token.type(), op1, op2, result);
                else
                    unsignedoperation(token.type(), op1, op2, result);
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
                if(stack.size() < 2)
                    return false;
                op2 = stack.top();
                stack.pop();
                op1 = stack.top();
                stack.pop();
                if(signedcalc)
                    signedoperation(token.type(), op1, op2, result);
                else
                    unsignedoperation(token.type(), op1, op2, result);
                stack.push(result);
                break;
            default: //do nothing
                break;
            }
        }
        else
        {
            uint result;
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