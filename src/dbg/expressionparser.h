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

        Token(const String & data, const Type type)
            : mData(data),
              mType(type)
        {
        }

        const String & data() const
        {
            return mData;
        }

        Type type() const
        {
            return mType;
        }

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
    void addOperatorToken(const String & data, Token::Type type);
    bool unsignedOperation(Token::Type type, duint op1, duint op2, duint & result) const;
    bool signedOperation(Token::Type type, dsint op1, dsint op2, duint & result) const;

    void addOperatorToken(char ch, Token::Type type)
    {
        String data;
        data.push_back(ch);
        addOperatorToken(data, type);
    }

    bool nextChEquals(size_t i, char ch) const
    {
        return i + 1 < mExpression.length() && mExpression[i + 1] == ch;
    }

    String mExpression;
    bool mIsValidExpression;
    std::vector<Token> mTokens;
    std::vector<Token> mPrefixTokens;
    String mCurToken;
};

#endif //_EXPRESSION_PARSER_H