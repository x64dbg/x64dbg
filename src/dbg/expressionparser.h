#ifndef _EXPRESSION_PARSER_H
#define _EXPRESSION_PARSER_H

#include "_global.h"
#include "value.h"

class ExpressionParser
{
public:
    explicit ExpressionParser(const String & expression);
    bool Calculate(duint & value, bool signedcalc, bool allowassign, bool silent = true, bool baseonly = false, int* value_size = nullptr, bool* isvar = nullptr, bool* hexonly = nullptr) const;

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
            Error,
            Data,
            QuotedData,
            Function,
            Comma,
            OpenBracket,
            CloseBracket,

            OperatorUnarySub,
            OperatorUnaryAdd,
            OperatorNot,
            OperatorMul,
            OperatorHiMul,
            OperatorDiv,
            OperatorMod,
            OperatorAdd,
            OperatorSub,
            OperatorShl,
            OperatorShr,
            OperatorRol,
            OperatorRor,
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
            OperatorLogicalImpl,
            OperatorAssign,
            OperatorAssignMul,
            OperatorAssignHiMul,
            OperatorAssignDiv,
            OperatorAssignMod,
            OperatorAssignAdd,
            OperatorAssignSub,
            OperatorAssignShl,
            OperatorAssignShr,
            OperatorAssignRol,
            OperatorAssignRor,
            OperatorAssignAnd,
            OperatorAssignXor,
            OperatorAssignOr,
            OperatorSuffixInc,
            OperatorSuffixDec,
            OperatorPrefixInc,
            OperatorPrefixDec
        };

        enum class Associativity
        {
            LeftToRight,
            RightToLeft,
            Unspecified
        };

        Token(const String & data, Type type)
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

    struct EvalValue
    {
        bool evaluated = false;
        bool isString = false;
        duint value = 0;
        String data;

        explicit EvalValue(duint value)
            : evaluated(true), value(value) {}

        explicit EvalValue(const String & data, bool isString)
            : evaluated(false), data(data), isString(isString) {}

        bool DoEvaluate(duint & result, bool silent = true, bool baseonly = false, int* value_size = nullptr, bool* isvar = nullptr, bool* hexonly = nullptr) const
        {
            if(evaluated)
            {
                if(value_size)
                    *value_size = sizeof(duint);
                if(isvar)
                    *isvar = false;
                if(hexonly)
                    *hexonly = false;
                result = value;
                return true;
            }
            return valfromstring_noexpr(data.c_str(), &result, silent, baseonly, value_size, isvar, hexonly);
        }
    };

private:
    static String fixClosingBrackets(const String & expression);
    Token::Type resolveQuotedData() const;
    bool isUnaryOperator() const;
    void tokenize();
    void shuntingYard();
    void addOperatorToken(const String & data, Token::Type type);
    bool unsignedOperation(Token::Type type, const EvalValue & op1, const EvalValue & op2, EvalValue & result, bool silent, bool baseonly, bool allowassign) const;
    bool signedOperation(Token::Type type, const EvalValue & op1, const EvalValue & op2, EvalValue & result, bool silent, bool baseonly, bool allowassign) const;

    void addOperatorToken(char ch, Token::Type type)
    {
        String data;
        data.push_back(ch);
        addOperatorToken(data, type);
    }

    bool tryEatNextCh(size_t & i, char ch) const
    {
        if(!(i + 1 < mExpression.length() && mExpression[i + 1] == ch))
            return false;
        i++;
        return true;
    }

    String mExpression;
    bool mIsValidExpression;
    std::vector<Token> mTokens;
    std::vector<Token> mPrefixTokens;
    String mCurToken;
    std::vector<bool> mCurTokenQuoted;
};

#endif //_EXPRESSION_PARSER_H