#ifndef _EXPRESSION_PARSER_H
#define _EXPRESSION_PARSER_H

#include "_global.h"

class ExpressionParser
{
public:
    explicit ExpressionParser(const String & expression);
    bool Calculate(duint & value, bool signedcalc, bool silent = true, bool baseonly = false, int* value_size = nullptr, bool* isvar = nullptr, bool* hexonly = nullptr) const;

    const String & GetExpression() const
    {
        return mExpression;
    }

    bool IsValidExpression() const
    {
        return mIsValidExpression;
    }

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
            OperatorOr,
            OperatorEqual,
            OperatorNotEqual,
            OperatorBigger,
            OperatorSmaller,
            OperatorBiggerEqual,
            OperatorSmallerEqual,
            OperatorLogicalAnd,
            OperatorLogicalOr,
            OperatorLogicalNot,
            Error
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
        String mData;
        Type mType;
    };

private:
    static String fixClosingBrackets(const String & expression);
    bool isUnaryOperator() const;
    void tokenize();
    void shuntingYard();
    void addOperatorToken(const char ch, const Token::Type type);
    bool unsignedOperation(const Token::Type type, const duint op1, const duint op2, duint & result) const;
    bool signedOperation(const Token::Type type, const dsint op1, const dsint op2, duint & result) const;

    String mExpression;
    bool mIsValidExpression;
    std::vector<Token> mTokens;
    std::vector<Token> mPrefixTokens;
    String mCurToken;
};

#endif //_EXPRESSION_PARSER_H