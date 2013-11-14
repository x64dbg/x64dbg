#include "math.h"
#include "value.h"

struct BRACKET_PAIR
{
    int openpos;
    int closepos;
    int layer;
    int isset; //0=free, 1=open, 2=close
};

struct EXPRESSION
{
    BRACKET_PAIR* pairs;
    int total_pairs;
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

int mathisoperator(char ch)
{
    if(ch=='(' or ch==')')
        return 1;
    else if(ch=='~')
        return 2;
    else if(ch=='*' or ch=='#' or ch=='/' or ch=='%')
        return 3;
    else if(ch=='+' or ch=='-')
        return 4;
    else if(ch=='<' or ch=='>')
        return 5;
    else if(ch=='&')
        return 6;
    else if(ch=='^')
        return 7;
    else if(ch=='|')
        return 8;
    return 0;
}

/*
mathformat:
- remove doubles
*/
void mathformat(char* text)
{
    int len=strlen(text);
    char* temp=(char*)emalloc(len+1);
    memset(temp, 0, len+1);
    for(int i=0,j=0; i<len; i++)
        if(mathisoperator(text[i])<3 or text[i]!=text[i+1])
            j+=sprintf(temp+j, "%c", text[i]);
    strcpy(text, temp);
    efree(temp);
}

/*
- check for math operators
*/
bool mathcontains(const char* text)
{
    int len=strlen(text);
    for(int i=0; i<len; i++)
        if(mathisoperator(text[i]))
            return true;
    return false;
}

#ifdef __MINGW64__
static inline unsigned long long umulhi(unsigned long long x, unsigned long long y)
{
    return (unsigned long long)(((__uint128_t)x*y)>>64);
}

static inline long long mulhi(long long x, long long y)
{
    return (long long)(((__int128_t)x*y)>>64);
}
#elif _WIN64
#include <intrin.h>
static inline unsigned long long umulhi(unsigned long long x, unsigned long long y)
{
	unsigned __int64 res;
	_umul128(x,y,&res);
	return res;
}

static inline long long mulhi(long long x, long long y)
{
	__int64 res;
	_mul128(x,y,&res);
	return res;
}
#else
static inline unsigned int umulhi(unsigned int x, unsigned int y)
{
    return (unsigned int)(((unsigned long long)x*y)>>32);
}

static inline int mulhi(int x, int y)
{
    return (int)(((long long)x*y)>>32);
}
#endif //__MINGW64__

bool mathdounsignedoperation(char op, uint left, uint right, uint* result)
{
    switch(op)
    {
    case '*':
        *result=left*right;
        return true;
    case '#':
        *result=umulhi(left, right);
        return true;
    case '/':
        if(right)
        {
            *result=left/right;
            return true;
        }
        return false;
    case '%':
        if(right)
        {
            *result=left%right;
            return true;
        }
        return false;
    case '+':
        *result=left+right;
        return true;
    case '-':
        *result=left-right;
        return true;
    case '<':
        *result=left<<right;
        return true;
    case '>':
        *result=left>>right;
        return true;
    case '&':
        *result=left&right;
        return true;
    case '^':
        *result=left^right;
        return true;
    case '|':
        *result=left|right;
        return true;
    }
    return false;
}

bool mathdosignedoperation(char op, sint left, sint right, sint* result)
{
    switch(op)
    {
    case '*':
        *result=left*right;
        return true;
    case '#':
        *result=mulhi(left, right);
        return true;
    case '/':
        if(right)
        {
            *result=left/right;
            return true;
        }
        return false;
    case '%':
        if(right)
        {
            *result=left%right;
            return true;
        }
        return false;
    case '+':
        *result=left+right;
        return true;
    case '-':
        *result=left-right;
        return true;
    case '<':
        *result=left<<right;
        return true;
    case '>':
        *result=left>>right;
        return true;
    case '&':
        *result=left&right;
        return true;
    case '^':
        *result=left^right;
        return true;
    case '|':
        *result=left|right;
        return true;
    }
    return false;
}

static void fillpair(EXPRESSION* expstruct, int pos, int layer)
{
    for(int i=0; i<expstruct->total_pairs; i++)
    {
        if(!expstruct->pairs[i].isset)
        {
            expstruct->pairs[i].layer=layer;
            expstruct->pairs[i].openpos=pos;
            expstruct->pairs[i].isset=1;
            break;
        }
        else if(expstruct->pairs[i].layer==layer and expstruct->pairs[i].isset==1)
        {
            expstruct->pairs[i].closepos=pos;
            expstruct->pairs[i].isset=2;
            break;
        }
    }
}


static int matchpairs(EXPRESSION* expstruct, char* expression, int endlayer)
{
    int layer=endlayer;
    int len=strlen(expression);
    for(int i=0; i<len; i++)
    {
        if(expression[i]=='(')
        {
            layer++;
            int pos=expression+i-expstruct->expression;
            fillpair(expstruct, pos, layer);
            i+=matchpairs(expstruct, expression+i+1, layer);
        }
        else if(expression[i]==')')
        {
            if(layer==endlayer)
            {
                int pos=expression+i-expstruct->expression;
                fillpair(expstruct, pos, layer);
                return i;
            }
            layer--;
        }

    }
    return 0;
}

static int expressionformat(char* exp)
{
    int len=strlen(exp);
    int open=0;
    int close=0;
    for(int i=0; i<len; i++)
    {
        if(exp[i]=='(')
            open++;
        else if(exp[i]==')')
            close++;
    }
    if(close>open)
        return -1;
    int add=open-close;
    if(add)
    {
        memset(exp+len, ')', add);
        exp[len+add]=0;
    }
    return open;
}

static void adjustpairs(EXPRESSION* exps, int cur_open, int cur_close, int cur_len, int new_len)
{
    for(int i=0; i<exps->total_pairs; i++)
    {
        if(exps->pairs[i].openpos>cur_open)
            exps->pairs[i].openpos+=new_len-cur_len;
        if(exps->pairs[i].closepos>cur_close)
            exps->pairs[i].closepos+=new_len-cur_len;
    }
}

static bool printlayer(char* exp, EXPRESSION* exps, int layer)
{
    for(int i=0; i<exps->total_pairs; i++)
    {
        if(exps->pairs[i].layer==layer)
        {
            char temp[256]="";
            char backup[256]="";

            int open=exps->pairs[i].openpos;
            int close=exps->pairs[i].closepos;
            int len=close-open;
            strncpy(temp, exp+open+1, len-1);

            strcpy(backup, exp+open+len+1);

            uint value;
            if(!mathfromstring(temp, &value, 0, 0))
                return false;

            adjustpairs(exps, open, close, len+1, sprintf(exp+open, "%X", value));

            if(*backup)
                strcat(exp, backup);

        }
    }
    return true;
}

bool mathhandlebrackets(char* expression)
{
    EXPRESSION expstruct;
    expstruct.expression=expression;
    int total_pairs=expressionformat(expression);
    if(total_pairs==-1)
        return false;
    else if(!total_pairs)
        return true;
    expstruct.total_pairs=total_pairs;

    expstruct.pairs=(BRACKET_PAIR*)emalloc(expstruct.total_pairs*sizeof(BRACKET_PAIR));
    memset(expstruct.pairs, 0, expstruct.total_pairs*sizeof(BRACKET_PAIR));
    matchpairs(&expstruct, expression, 0);
    int deepest=0;
    for(int i=0; i<expstruct.total_pairs; i++)
        if(expstruct.pairs[i].layer>deepest)
            deepest=expstruct.pairs[i].layer;

    for(int i=deepest; i>0; i--)
        if(!printlayer(expression, &expstruct, i))
            return false;

    efree(expstruct.pairs);
    return true;
}

/*
- handle math
*/
bool mathfromstring(const char* string, uint* value, int* value_size, bool* isvar)
{
    int highestop=0;
    int highestop_pos=0;
    int len=strlen(string);
    for(int i=0; i<len; i++)
    {
        int curop=mathisoperator(string[i]);
        if(curop>1 and curop>highestop)
        {
            highestop=curop;
            highestop_pos=i;
        }
    }
    if(!highestop)
    {
        if(!valfromstring(string, value, value_size, isvar, false, 0))
            return false;
        return true;
    }
    char* strleft=(char*)emalloc(len+1);
    char* strright=(char*)emalloc(len+1);
    memset(strleft, 0, len+1);
    memset(strright, 0, len+1);
    strncpy(strleft, string, highestop_pos);
    strcpy(strright, string+highestop_pos+1);
    //dprintf("left: %s, right: %s, op: %c\n", strleft, strright, string[highestop_pos]);
    if(!*strright)
    {
        efree(strleft);
        efree(strright);
        return false;
    }
    uint right=0;
    if(!valfromstring(strright, &right, 0, 0, false, 0))
    {
        efree(strleft);
        efree(strright);
        return false;
    }
    if(string[highestop_pos]=='~')
    {
        right=~right;
        if(!strlen(strleft))
        {
            *value=right;
            efree(strleft);
            efree(strright);
            return true;
        }
    }
    uint left=0;
    if(!valfromstring(strleft, &left, 0, 0, false, 0))
    {
        efree(strleft);
        efree(strright);
        return false;
    }
    bool math_ok;
    if(valuesignedcalc())
        math_ok=mathdosignedoperation(string[highestop_pos], left, right, (sint*)value);
    else
        math_ok=mathdounsignedoperation(string[highestop_pos], left, right, value);
    efree(strleft);
    efree(strright);
    return math_ok;
}

