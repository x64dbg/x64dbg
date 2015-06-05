/**
 @file math.cpp

 @brief Implements various functionalities that have to do with handling expression text.
 */

#include "math.h"
#include "value.h"

/**
\brief A bracket pair. This structure describes a piece of brackets '(' and ')'
*/
struct BRACKET_PAIR
{
    /**
    \brief The position in the string of the opening bracket '('.
    */
    int openpos;

    /**
    \brief The position in the string of the closing bracket ')'.
    */
    int closepos;

    /**
    \brief The depth of the pair (for example when you have "((1+4)*4)" the second '(' has layer 2).
    */
    int layer;

    /**
    \brief 0 when there is nothing set, 1 when the openpos is set, 2 when everything is set (aka pair is complete).
    */
    int isset;
};

/**
\brief An expression. This structure describes an expression in form of bracket pairs.
*/
struct EXPRESSION
{
    /**
    \brief The bracket pairs.
    */
    BRACKET_PAIR* pairs;

    /**
    \brief The total number of bracket pairs.
    */
    int total_pairs;

    /**
    \brief The expression text everything is derived from.
    */
    char* expression;
};

/**
\brief Determines if a characters is an operator.
\param ch The character to check.
\return The number of the operator. 0 when the character is no operator. Otherwise it returns one of the following numbers:

- 1 ( )
- 2 ~     (NOT)
- 3 * / % (MUL DIV)
- 4 + -   (ADD SUB)
- 5 < >   (SHL SHR)
- 6 &     (AND)
- 7 ^     (XOR)
- 8 |     (OR)
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

/**
\brief Formats the given text. It removes double operators like "**" and "||"
\param [in,out] text The text to format.
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

/**
\brief Checks if the given text contains operators.
\param text The text to check.
\return true if the text contains operator, false if not.
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
\brief Do an operation on two unsigned numbers.
\param op The operation to do (this must be a valid operator).
\param left The number left of the operator.
\param right The number right of the operator.
\param [out] result The result of the operator. Cannot be null.
\return true if the operation succeeded. It could fail on zero devision or an invalid operator.
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
\brief Do an operation on two signed numbers.
\param op The operation to do (this must be a valid operator).
\param left The number left of the operator.
\param right The number right of the operator.
\param [out] result The result of the operator. Cannot be null.
\return true if the operation succeeded. It could fail on zero devision or an invalid operator.
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
\brief Fills a BRACKET_PAIR structure.
\param [in,out] expstruct The expression structure. Cannot be null.
\param pos The position to fill, position of '(' or ')'.
\param layer The layer this bracket is in.
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
\brief This function recursively matches bracket pair in an EXPRESSION.
\param [in,out] expstruct The expression structure. Cannot be null.
\param [in,out] expression The expression text to parse. Cannot be null.
\param endlayer The layer to stop on. This variable is used for the recursion termination condition.
\return The position in the \p expression mathpairs ended in.
*/
static int matchpairs(EXPRESSION* expstruct, char* expression, int endlayer = 0)
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
\brief Formats a given expression. This function checks if the number of brackets is even and adds brackets to the end if needed.
\param [in,out] exp The expression to format.
\return The number of bracket pairs in the expression or -1 on error.
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
\brief Adjusts bracket pair positions to insert a new string in the expression.
\param [in,out] exps The expression structure.
\param cur_open The current opening bracket '(' position.
\param cur_close The current closing bracket ')' position.
\param cur_len The current string length in between the brackets.
\param new_len Length of the new string.
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
\brief Prints value of expressions in between brackets on a certain bracket layer (expression is resolved using mathfromstring(), which means the whole thing can work recursively).
\param [in,out] exp The expression to print. Cannot be null.
\param [in,out] exps The expression structure. Cannot be null.
\param layer The layer to print.
\param silent Value to pass on to mathfromstring().
\param baseonly Value to pass on to mathfromstring().
\return true if printing the layer was succesful, false otherwise.
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
\brief Handle brackets in an expression (calculate the values of expressions in between brackets).
\param [in,out] expression Expression to handle. Cannot be null.
\param silent Value to pass on to printlayer().
\param baseonly Value to pass on to printlayer().
\return true if the brackets are correctly expanded, false otherwise.
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
    matchpairs(&expstruct, expression);
    int deepest = 0;
    for(int i = 0; i < expstruct.total_pairs; i++)
        if(expstruct.pairs[i].layer > deepest)
            deepest = expstruct.pairs[i].layer;

    for(int i = deepest; i > 0; i--)
        if(!printlayer(expression, &expstruct, i, silent, baseonly))
            return false;
    return true;
}

/**
\brief Calculate the value of an expression string.
\param string The string to calculate the value of. Cannot be null.
\param [in,out] value The resulting value. Cannot be null.
\param silent Value to pass on to valfromstring().
\param baseonly Value to pass on to valfromstring().
\param [in,out] value_size Value to pass on to valfromstring(). Can be null.
\param [in,out] isvar Value to pass on to valfromstring(). Can be null.
\return true if the string was successfully parsed and the value was calculated.
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
