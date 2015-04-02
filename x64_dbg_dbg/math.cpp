#include "math.h"
#include "value.h"

enum BRACKET_TYPE
{
	BRACKET_FREE,
	BRACKET_OPEN,
	BRACKET_CLOSE,
};

struct BRACKET_PAIR
{
    int openpos;
    int closepos;
    int layer;
    BRACKET_TYPE isset;
};

struct EXPRESSION
{
    BRACKET_PAIR* pairs;
    int total_pairs;
    char* expression;
};

/*
operator precedence
0       (INVALID/NONE)
1 ( )   (PARENTHESIS)
2 ~     (NOT)
3 * / % (MUL DIV)
4 + -   (ADD SUB)
5 < >   (SHL SHR)
6 &     (AND)
7 ^     (XOR)
8 |     (OR)
*/
int mathisoperator(char ch)
{
	//
	// The lower the number, the higher the priority.
	// Zero indicates no operator was found.
	//
    if(ch == '(' || ch == ')')
        return 1;
    else if(ch == '~')
        return 2;
    else if(ch == '*' || ch == '`' || ch == '/' || ch == '%')
        return 3;
    else if(ch == '+' || ch == '-')
        return 4;
    else if(ch == '<' || ch == '>')
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
void mathformat(char* text)
{
    int len = (int)strlen(text);
    Memory<char*> temp(len + 1, "mathformat:temp");

	for (int i = 0, j = 0; i < len; i++)
	{
		if (mathisoperator(text[i]) < 3 || text[i] != text[i + 1])
			j += sprintf(temp + j, "%c", text[i]);
	}

    strcpy(text, temp);
}

/*
- check for math operators
*/
bool mathcontains(const char* text)
{
	// Skip negative values
    if(*text == '-')
        text++;

	// Search the entire string looking for a math operator
	for (; text[0] != '\0'; text++)
	{
		if (mathisoperator(text[0]))
			return true;
	}

    return false;
}

#ifdef __MINGW64__
inline unsigned long long umulhi(unsigned long long x, unsigned long long y)
{
    return (unsigned long long)(((__uint128_t)x * y) >> 64);
}

inline long long mulhi(long long x, long long y)
{
    return (long long)(((__int128_t)x * y) >> 64);
}
#elif _WIN64
#include <intrin.h>
inline unsigned long long umulhi(unsigned long long x, unsigned long long y)
{
    unsigned __int64 res;
    _umul128(x, y, &res);
    return res;
}

inline long long mulhi(long long x, long long y)
{
    __int64 res;
    _mul128(x, y, &res);
    return res;
}
#else
inline unsigned int umulhi(unsigned int x, unsigned int y)
{
    return (unsigned int)(((unsigned long long)x * y) >> 32);
}

inline int mulhi(int x, int y)
{
    return (int)(((long long)x * y) >> 32);
}
#endif //__MINGW64__

template<typename T>
bool MathDoOperation(char op, T left, T right, T* result)
{
	switch (op)
	{
	case '*':
		*result = left * right;
		return true;
	case '`':
		*result = umulhi(left, right);
		return true;
	case '/':
		if (right)
		{
			*result = left / right;
			return true;
		}
		return false;
	case '%':
		if (right)
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

bool mathdounsignedoperation(char op, uint left, uint right, uint* result)
{
	return MathDoOperation<uint>(op, left, right, result);
}

bool mathdosignedoperation(char op, sint left, sint right, sint* result)
{
	return MathDoOperation<sint>(op, left, right, result);
}

void fillpair(EXPRESSION* expstruct, int pos, int layer)
{
    for(int i = 0; i < expstruct->total_pairs; i++)
    {
        if(expstruct->pairs[i].isset == BRACKET_FREE)
        {
            expstruct->pairs[i].layer	= layer;
            expstruct->pairs[i].openpos	= pos;
            expstruct->pairs[i].isset	= BRACKET_OPEN;
            break;
        }
        else if(expstruct->pairs[i].layer == layer && expstruct->pairs[i].isset == BRACKET_OPEN)
        {
            expstruct->pairs[i].closepos	= pos;
            expstruct->pairs[i].isset		= BRACKET_CLOSE;
            break;
        }
    }
}

int matchpairs(EXPRESSION* expstruct, char* expression, int endlayer)
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

int expressionformat(char* exp)
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

void adjustpairs(EXPRESSION* exps, int cur_open, int cur_close, int cur_len, int new_len)
{
    for(int i = 0; i < exps->total_pairs; i++)
    {
        if(exps->pairs[i].openpos > cur_open)
            exps->pairs[i].openpos += new_len - cur_len;

        if(exps->pairs[i].closepos > cur_close)
            exps->pairs[i].closepos += new_len - cur_len;
    }
}

bool printlayer(char* exp, EXPRESSION* exps, int layer, bool silent, bool baseonly)
{
    for(int i = 0; i < exps->total_pairs; i++)
    {
        if(exps->pairs[i].layer == layer)
        {
            int open	= exps->pairs[i].openpos;
            int close	= exps->pairs[i].closepos;
            int len		= close - open;

			char temp[256];
            strncpy(temp, exp + open + 1, len - 1);

			char backup[256];
            strcpy_s(backup, exp + open + len + 1);

            uint value;
            if(!mathfromstring(temp, &value, silent, baseonly, nullptr, nullptr))
                return false;

            adjustpairs(exps, open, close, len + 1, sprintf(exp + open, "%"fext"X", value));

            if(*backup)
                strcat(exp, backup);
        }
    }

    return true;
}

bool mathhandlebrackets(char* expression, bool silent, bool baseonly)
{
    int totalPairs = expressionformat(expression);

    if(totalPairs == -1)
        return false;
    else if(!totalPairs)
        return true;

    Memory<BRACKET_PAIR*> pairs(totalPairs * sizeof(BRACKET_PAIR), "mathhandlebrackets:pairs");

	EXPRESSION expStruct;
	expStruct.expression	= expression;
	expStruct.total_pairs	= totalPairs;
	expStruct.pairs			= pairs;

    matchpairs(&expStruct, expression, 0);

    int deepest = 0;
	for (int i = 0; i < expStruct.total_pairs; i++)
	{
		if (expStruct.pairs[i].layer > deepest)
			deepest = expStruct.pairs[i].layer;
	}

	for (int i = deepest; i > 0; i--)
	{
		if (!printlayer(expression, &expStruct, i, silent, baseonly))
			return false;
	}

    return true;
}

/*
- handle math
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

