#ifndef _EXPRESSION_PARSER_H
#define _EXPRESSION_PARSER_H

#include "_global.h"

class ExpressionParser
{
public:
    ExpressionParser(const String & expression);
    bool calculate(uint & value, bool signedcalc, bool silent, bool baseonly, int* value_size, bool* isvar, bool* hexonly);

    class Token
    {
    public:
        enum class Type
        {
            Data,
            OpenBracket,
            CloseBracket,
            OperatorUnarySub,
            OperatorNot,
            OperatorMul,
            OperatorHiMul,
            OperatorDiv,
            OperatorMod,
            OperatorAdd,
            OperatorSub,
            OperatorShl,
            OperatorShr,
            OperatorAnd,
            OperatorXor,
            OperatorOr
        };

        enum class Associativity
        {
            LeftToRight,
            RightToLeft,
            Unspecified
        };

        Token(const String & data, const Type type);
        const String & data() const;
        Type type() const;
        Associativity associativity() const;
        int precedence() const;
        bool isOperator() const;

    private:
        String _data;
        Type _type;
    };

private:
    String fixClosingBrackets(const String & expression);
    bool isUnaryOperator();
    void tokenize(const String & expression);
    void shuntingYard();
    void addOperatorToken(const char ch, const Token::Type type);
    bool unsignedoperation(const Token::Type type, const uint op1, const uint op2, uint & result);
    bool signedoperation(const Token::Type type, const sint op1, const sint op2, uint & result);

    std::vector<Token> _tokens;
    std::vector<Token> _prefixTokens;
    String _curToken;
};

#endif //_EXPRESSION_PARSER_H