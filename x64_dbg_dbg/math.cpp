/**
 @file math.cpp

 @brief Implements the mathematics class.
 */

#include "math.h"
#include "value.h"

/**
 @struct BRACKET_PAIR

 @brief A bracket pair.
 */

struct BRACKET_PAIR
{
    /**
     @brief The openpos.
     */

    int openpos;

    /**
     @brief The closepos.
     */

    int closepos;

    /**
     @brief The layer.
     */

    int layer;

    /**
     @brief 0=free, 1=open, 2=close.
     */

    int isset;
};

/**
 @struct EXPRESSION

 @brief An expression.
 */

struct EXPRESSION
{
    /**
     @brief The pairs.
     */

    BRACKET_PAIR* pairs;

    /**
     @brief The total pairs.
     */

    int total_pairs;

    /**
     @brief The expression.
     */

    char* expression;
};

/*
operator precedence
1 ( )
2 ~     (NOT)
3 * / % (MUL DIV)
4 + -   (ADD SUB)
5 < >   (SHL SHR)
6 &     (AND)
7 ^     (XOR)
8 |     (OR)
*/

/**
 @fn int mathisoperator(char ch)

 @brief  casting operator.

 @param ch The ch.

 @return The result of the operation.
 */

int mathisoperator(char ch)
{
    if(ch == '(' or ch == ')')
        return 1;
    else if(ch == '~')
        return 2;
    else if(ch == '*' or ch == '`' or ch == '/' or ch == '%')
        return 3;
    else if(ch == '+' or ch == '-')
        return 4;
    else if(ch == '<' or ch == '>')
        return 5;
    else if(ch == '&')
        return 6;
    else if(ch == '^')
        return 7;
    else if(ch == '|')
        return 8;
    return 0;
}

/*
mathformat:
- remove doubles
*/

/**
 @fn void mathformat(char* text)

 @brief Mathformats the given text.

 @param [in,out] text If non-null, the text.
 */

void mathformat(char* text)
{
    int len = (int)strlen(text);
    Memory<char*> temp(len + 1, "mathformat:temp");
    memset(temp, 0, len + 1);
    for(int i = 0, j = 0; i < len; i++)
        if(mathisoperator(text[i]) < 3 or text[i] != text[i + 1])
            j += sprintf(temp + j, "%c", text[i]);
    strcpy(text, temp);
}

/*
- check for math operators
*/

/**
 @fn bool mathcontains(const char* text)

 @brief Mathcontains the given text.

 @param text The text.

 @return true if it succeeds, false if it fails.
 */

bool mathcontains(const char* text)
{
    if(*text == '-') //ignore negative values
        text++;
    int len = (int)strlen(text);
    for(int i = 0; i < len; i++)
        if(mathisoperator(text[i]))
            return true;
    return false;
}

#ifdef __MINGW64__
static inline unsigned long long umulhi(unsigned long long x, unsigned long long y)
{
    return (unsigned long long)(((__uint128_t)x * y) >> 64);
}

static inline long long mulhi(long long x, long long y)
{
    return (long long)(((__int128_t)x * y) >> 64);
}
#elif _WIN64
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

/**
 @fn bool mathdounsignedoperation(char op, uint left, uint right, uint* result)

 @brief Mathdounsignedoperations.

 @param op           The operation.
 @param left         The left.
 @param right        The right.
 @param [out] result If non-null, the result.

 @return true if it succeeds, false if it fails.
 */

bool mathdounsignedoperation(char op, uint left, uint right, uint* result)
{
    switch(op)
    {
    case '*':
        *result = left * right;
        return true;
    case '`':
        *result = umulhi(left, right);
        return true;
    case '/':
        if(right)
        {
            *result = left / right;
            return true;
        }
        return false;
    case '%':
        if(right)
        {
            *result = left % right;
            return true;
        }
        return false;
    case '+':
        *result = left + right;
        return true;
    case '-':
        *result = left - right;
        return true;
    case '<':
        *result = left << right;
        return true;
    case '>':
        *result = left >> right;
        return true;
    case '&':
        *result = left & right;
        return true;
    case '^':
        *result = left ^ right;
        return true;
    case '|':
        *result = left | right;
        return true;
    }
    return false;
}

/**
 @fn bool mathdosignedoperation(char op, sint left, sint right, sint* result)

 @brief Mathdosignedoperations.

 @param op           The operation.
 @param left         The left.
 @param right        The right.
 @param [out] result If non-null, the result.

 @return true if it succeeds, false if it fails.
 */

bool mathdosignedoperation(char op, sint left, sint right, sint* result)
{
    switch(op)
    {
    case '*':
        *result = left * right;
        return true;
    case '`':
        *result = mulhi(left, right);
        return true;
    case '/':
        if(right)
        {
            *result = left / right;
            return true;
        }
        return false;
    case '%':
        if(right)
        {
            *result = left % right;
            return true;
        }
        return false;
    case '+':
        *result = left + right;
        return true;
    case '-':
        *result = left - right;
        return true;
    case '<':
        *result = left << right;
        return true;
    case '>':
        *result = left >> right;
        return true;
    case '&':
        *result = left & right;
        return true;
    case '^':
        *result = left ^ right;
        return true;
    case '|':
        *result = left | right;
        return true;
    }
    return false;
}

/**
 @fn static void fillpair(EXPRESSION* expstruct, int pos, int layer)

 @brief Fillpairs.

 @param [in,out] expstruct If non-null, the expstruct.
 @param pos                The position.
 @param layer              The layer.
 */

static void fillpair(EXPRESSION* expstruct, int pos, int layer)
{
    for(int i = 0; i < expstruct->total_pairs; i++)
    {
        if(!expstruct->pairs[i].isset)
        {
            expstruct->pairs[i].layer = layer;
            expstruct->pairs[i].openpos = pos;
            expstruct->pairs[i].isset = 1;
            break;
        }
        else if(expstruct->pairs[i].layer == layer and expstruct->pairs[i].isset == 1)
        {
            expstruct->pairs[i].closepos = pos;
            expstruct->pairs[i].isset = 2;
            break;
        }
    }
}

/**
 @fn static int matchpairs(EXPRESSION* expstruct, char* expression, int endlayer)

 @brief Matchpairs.

 @param [in,out] expstruct  If non-null, the expstruct.
 @param [in,out] expression If non-null, the expression.
 @param endlayer            The endlayer.

 @return An int.
 */

static int matchpairs(EXPRESSION* expstruct, char* expression, int endlayer)
{
    int layer = endlayer;
    int len = (int)strlen(expression);
    for(int i = 0; i < len; i++)
    {
        if(expression[i] == '(')
        {
            layer++;
            int pos = (int)(expression + i - expstruct->expression);
            fillpair(expstruct, pos, layer);
            i += matchpairs(expstruct, expression + i + 1, layer);
        }
        else if(expression[i] == ')')
        {
            if(layer == endlayer)
            {
                int pos = (int)(expression + i - expstruct->expression);
                fillpair(expstruct, pos, layer);
                return i;
            }
            layer--;
        }

    }
    return 0;
}

/**
 @fn static int expressionformat(char* exp)

 @brief Expressionformats the given exponent.

 @param [in,out] exp If non-null, the exponent.

 @return An int.
 */

static int expressionformat(char* exp)
{
    int len = (int)strlen(exp);
    int open = 0;
    int close = 0;
    for(int i = 0; i < len; i++)
    {
        if(exp[i] == '(')
            open++;
        else if(exp[i] == ')')
            close++;
    }
    if(close > open)
        return -1;
    int add = open - close;
    if(add)
    {
        memset(exp + len, ')', add);
        exp[len + add] = 0;
    }
    return open;
}

/**
 @fn static void adjustpairs(EXPRESSION* exps, int cur_open, int cur_close, int cur_len, int new_len)

 @brief Adjustpairs.

 @param [in,out] exps If non-null, the exps.
 @param cur_open      The current open.
 @param cur_close     The current close.
 @param cur_len       The current length.
 @param new_len       Length of the new.
 */

static void adjustpairs(EXPRESSION* exps, int cur_open, int cur_close, int cur_len, int new_len)
{
    for(int i = 0; i < exps->total_pairs; i++)
    {
        if(exps->pairs[i].openpos > cur_open)
            exps->pairs[i].openpos += new_len - cur_len;
        if(exps->pairs[i].closepos > cur_close)
            exps->pairs[i].closepos += new_len - cur_len;
    }
}

/**
 @fn static bool printlayer(char* exp, EXPRESSION* exps, int layer, bool silent, bool baseonly)

 @brief Printlayers.

 @param [in,out] exp  If non-null, the exponent.
 @param [in,out] exps If non-null, the exps.
 @param layer         The layer.
 @param silent        true to silent.
 @param baseonly      true to baseonly.

 @return true if it succeeds, false if it fails.
 */

static bool printlayer(char* exp, EXPRESSION* exps, int layer, bool silent, bool baseonly)
{
    for(int i = 0; i < exps->total_pairs; i++)
    {
        if(exps->pairs[i].layer == layer)
        {
            char temp[256] = "";
            char backup[256] = "";

            int open = exps->pairs[i].openpos;
            int close = exps->pairs[i].closepos;
            int len = close - open;
            strncpy(temp, exp + open + 1, len - 1);

            strcpy_s(backup, exp + open + len + 1);

            uint value;
            if(!mathfromstring(temp, &value, silent, baseonly, 0, 0))
                return false;

            adjustpairs(exps, open, close, len + 1, sprintf(exp + open, "%"fext"X", value));

            if(*backup)
                strcat(exp, backup);

        }
    }
    return true;
}

/**
 @fn bool mathhandlebrackets(char* expression, bool silent, bool baseonly)

 @brief Mathhandlebrackets.

 @param [in,out] expression If non-null, the expression.
 @param silent              true to silent.
 @param baseonly            true to baseonly.

 @return true if it succeeds, false if it fails.
 */

bool mathhandlebrackets(char* expression, bool silent, bool baseonly)
{
    EXPRESSION expstruct;
    expstruct.expression = expression;
    int total_pairs = expressionformat(expression);
    if(total_pairs == -1)
        return false;
    else if(!total_pairs)
        return true;
    expstruct.total_pairs = total_pairs;

    Memory<BRACKET_PAIR*> pairs(expstruct.total_pairs * sizeof(BRACKET_PAIR), "mathhandlebrackets:expstruct.pairs");
    expstruct.pairs = pairs;
    memset(expstruct.pairs, 0, expstruct.total_pairs * sizeof(BRACKET_PAIR));
    matchpairs(&expstruct, expression, 0);
    int deepest = 0;
    for(int i = 0; i < expstruct.total_pairs; i++)
        if(expstruct.pairs[i].layer > deepest)
            deepest = expstruct.pairs[i].layer;

    for(int i = deepest; i > 0; i--)
        if(!printlayer(expression, &expstruct, i, silent, baseonly))
            return false;
    return true;
}

/*
- handle math
*/

/**
 @fn bool mathfromstring(const char* string, uint* value, bool silent, bool baseonly, int* value_size, bool* isvar)

 @brief Mathfromstrings.

 @param string              The string.
 @param [in,out] value      If non-null, the value.
 @param silent              true to silent.
 @param baseonly            true to baseonly.
 @param [in,out] value_size If non-null, size of the value.
 @param [in,out] isvar      If non-null, the isvar.

 @return true if it succeeds, false if it fails.
 */

bool mathfromstring(const char* string, uint* value, bool silent, bool baseonly, int* value_size, bool* isvar)
{
    int highestop = 0;
    int highestop_pos = 0;
    int len = (int)strlen(string);
    bool negative = false;
    if(*string == '-')
    {
        negative = true;
        string++;
    }
    for(int i = 0; i < len; i++)
    {
        int curop = mathisoperator(string[i]);
        if(curop > 1 and curop > highestop)
        {
            highestop = curop;
            highestop_pos = i;
        }
    }
    if(!highestop)
        return valfromstring(string, value, silent, baseonly, value_size, isvar, 0);
    Memory<char*> strleft(len + 1 + negative, "mathfromstring:strleft");
    Memory<char*> strright(len + 1, "mathfromstring:strright");
    memset(strleft, 0, len + 1);
    memset(strright, 0, len + 1);
    strncpy(strleft, string - negative, highestop_pos + negative);
    strcpy(strright, string + highestop_pos + 1);
    strcpy(strleft, StringUtils::Trim(strleft).c_str());
    strcpy(strright, StringUtils::Trim(strright).c_str());
    //dprintf("left: %s, right: %s, op: %c\n", strleft(), strright(), string[highestop_pos]);
    if(!*strright)
        return false;
    uint right = 0;
    if(!valfromstring(strright, &right, silent, baseonly))
        return false;
    if(string[highestop_pos] == '~')
    {
        right = ~right;
        if(!strlen(strleft))
        {
            *value = right;
            return true;
        }
    }
    uint left = 0;
    if(!valfromstring(strleft, &left, silent, baseonly))
        return false;
    bool math_ok;
    if(valuesignedcalc())
        math_ok = mathdosignedoperation(string[highestop_pos], left, right, (sint*)value);
    else
        math_ok = mathdounsignedoperation(string[highestop_pos], left, right, value);
    return math_ok;
}

