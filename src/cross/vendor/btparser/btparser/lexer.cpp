#include "lexer.h"
#include "helpers.h"

#include <cctype>

#define MAKE_OP_TRIPLE(ch1, ch2, ch3) (ch3 << 16 | ch2 << 8 | ch1)
#define MAKE_OP_DOUBLE(ch1, ch2) (ch2 << 8 | ch1)
#define MAKE_OP_SINGLE(ch1) (ch1)
#define DEFAULT_STRING_BUFFER 65536

static void clearReserve(std::string & str, size_t reserve = DEFAULT_STRING_BUFFER)
{
    str.clear();
    str.reserve(reserve);
}

static const char* convertNumber(const char* str, uint64_t & result, int radix)
{
    errno = 0;
    char* end;
    result = strtoull(str, &end, radix);
    if(!result && end == str)
        return "not a number";
    if(result == ULLONG_MAX && errno)
        return "does not fit";
    if(*end)
        return "str not completely consumed";
    return nullptr;
}

Lexer::Lexer()
{
    setupTokenMaps();
}

bool Lexer::ReadInputFile(const std::string & filename)
{
    resetLexerState();
    return FileHelper::ReadAllData(filename, mInput);
}

void Lexer::SetInputData(const std::string & data)
{
    resetLexerState();
    for(auto & ch : data)
        mInput.push_back(ch);
}

bool Lexer::DoLexing(std::vector<TokenState> & tokens, std::string & error)
{
    while(true)
    {
        size_t lineIndex = -1;
        auto token = getToken(lineIndex);
        if (lineIndex == -1)
            __debugbreak();
        mState.Token = token;
        if(token == tok_error)
        {
            error = StringUtils::sprintf("line %d, col %d: %s", mState.CurLine + 1, mState.LineIndex, mState.ErrorMessage.c_str());
            return false;
        }
        tokens.push_back(mState);
        // Restore the line index from when we started parsing the token
        tokens.back().LineIndex = lineIndex;
        mState.Clear();
        if(token == tok_eof)
            break;
    }
    return true;
}

bool Lexer::Test(const std::function<void(const std::string & line)> & lexEnum, bool output)
{
    size_t line = 0;
    if(output)
        lexEnum("1: ");
    Token tok;
    std::string toks;
    clearReserve(toks);
    char newlineText[128] = "";
    do
    {
        size_t lineIndex = -1;
        tok = getToken(lineIndex);
        if(!output)
            continue;
        toks.clear();
        while(line < mState.CurLine)
        {
            line++;
            sprintf_s(newlineText, "\n%zu: ", line + 1);
            toks.append(newlineText);
        }
        toks.append(TokString(tok));
        toks.push_back(' ');
        lexEnum(toks);
    }
    while(tok != tok_eof && tok != tok_error);
    for(const auto & warning : mWarnings)
        if(output)
            lexEnum("\nwarning: " + warning);
    return tok != tok_error;
}

Lexer::Token Lexer::getToken(size_t & tokenLineIndex)
{
    //skip whitespace
    while(isspace(mLastChar))
    {
        if(mLastChar == '\n')
            signalNewLine();
        nextChar();
    }

    //skip \\[\r\n]
    if(mLastChar == '\\' && (peekChar() == '\r' || peekChar() == '\n'))
    {
        nextChar();
        return getToken(tokenLineIndex);
    }

    //character literal
    if(mLastChar == '\'')
    {
        tokenLineIndex = mState.LineIndex - 1;
        std::string charLit;
        while(true)
        {
            nextChar();
            if(mLastChar == EOF) //end of file
                return reportError("unexpected end of file in character literal (1)");
            if(mLastChar == '\r' || mLastChar == '\n')
                return reportError("unexpected newline in character literal (1)");
            if(mLastChar == '\'') //end of character literal
            {
                if(charLit.length() != 1)
                    return reportError(StringUtils::sprintf("invalid character literal '%s'", charLit.c_str()));
                mState.CharLit = charLit[0];
                nextChar();
                return tok_charlit;
            }
            if(mLastChar == '\\') //escape sequence
            {
                nextChar();
                if(mLastChar == EOF)
                    return reportError("unexpected end of file in character literal (2)");
                if(mLastChar == '\r' || mLastChar == '\n')
                    return reportError("unexpected newline in character literal (2)");
                if(mLastChar == '\'' || mLastChar == '\"' || mLastChar == '?' || mLastChar == '\\')
                    ;
                else if(mLastChar == 'a')
                    mLastChar = '\a';
                else if(mLastChar == 'b')
                    mLastChar = '\b';
                else if(mLastChar == 'f')
                    mLastChar = '\f';
                else if(mLastChar == 'n')
                    mLastChar = '\n';
                else if(mLastChar == 'r')
                    mLastChar = '\r';
                else if(mLastChar == 't')
                    mLastChar = '\t';
                else if(mLastChar == 'v')
                    mLastChar = '\v';
                else if(mLastChar == '0')
                    mLastChar = '\0';
                else if(mLastChar == 'x') //\xHH
                {
                    auto ch1 = nextChar();
                    auto ch2 = nextChar();
                    if(isxdigit(ch1) && isxdigit(ch2))
                    {
                        char byteStr[3] = "";
                        byteStr[0] = ch1;
                        byteStr[1] = ch2;
                        uint64_t hexData;
                        auto error = convertNumber(byteStr, hexData, 16);
                        if(error)
                            return reportError(StringUtils::sprintf("convertNumber failed (%s) for hex sequence \"\\x%c%c\" in character literal", error, ch1, ch2));
                        mLastChar = hexData & 0xFF;
                    }
                    else
                        return reportError(StringUtils::sprintf("invalid hex sequence \"\\x%c%c\" in character literal", ch1, ch2));
                }
                else
                    return reportError(StringUtils::sprintf("invalid escape sequence \"\\%c\" in character literal", mLastChar));
            }
            charLit += mLastChar;
        }
    }

    //string literal
    if(mLastChar == '\"')
    {
        tokenLineIndex = mState.LineIndex - 1;
        mState.StringLit.clear();
        while(true)
        {
            nextChar();
            if(mLastChar == EOF) //end of file
                return reportError("unexpected end of file in string literal (1)");
            if(mLastChar == '\r' || mLastChar == '\n')
                return reportError("unexpected newline in string literal (1)");
            if(mLastChar == '\"') //end of string literal
            {
                nextChar();
                return tok_stringlit;
            }
            if(mLastChar == '\\') //escape sequence
            {
                nextChar();
                if(mLastChar == EOF)
                    return reportError("unexpected end of file in string literal (2)");
                if(mLastChar == '\r' || mLastChar == '\n')
                    return reportError("unexpected newline in string literal (2)");
                if(mLastChar == '\'' || mLastChar == '\"' || mLastChar == '?' || mLastChar == '\\')
                    ;
                else if(mLastChar == 'a')
                    mLastChar = '\a';
                else if(mLastChar == 'b')
                    mLastChar = '\b';
                else if(mLastChar == 'f')
                    mLastChar = '\f';
                else if(mLastChar == 'n')
                    mLastChar = '\n';
                else if(mLastChar == 'r')
                    mLastChar = '\r';
                else if(mLastChar == 't')
                    mLastChar = '\t';
                else if(mLastChar == 'v')
                    mLastChar = '\v';
                else if(mLastChar == '0')
                    mLastChar = '\0';
                else if(mLastChar == 'x') //\xHH
                {
                    auto ch1 = nextChar();
                    auto ch2 = nextChar();
                    if(isxdigit(ch1) && isxdigit(ch2))
                    {
                        char byteStr[3] = "";
                        byteStr[0] = ch1;
                        byteStr[1] = ch2;
                        uint64_t hexData;
                        auto error = convertNumber(byteStr, hexData, 16);
                        if(error)
                            return reportError(StringUtils::sprintf("convertNumber failed (%s) for hex sequence \"\\x%c%c\" in string literal", error, ch1, ch2));
                        mLastChar = hexData & 0xFF;
                    }
                    else
                        return reportError(StringUtils::sprintf("invalid hex sequence \"\\x%c%c\" in string literal", ch1, ch2));
                }
                else
                    return reportError(StringUtils::sprintf("invalid escape sequence \"\\%c\" in string literal", mLastChar));
            }
            mState.StringLit.push_back(mLastChar);
        }
    }

    //identifier/keyword
    if(isalpha(mLastChar) || mLastChar == '_' || (mLastChar == ':' && peekChar() == ':')) //[a-zA-Z_]
    {
        tokenLineIndex = mState.LineIndex - 1;
        mState.IdentifierStr = mLastChar;
        if (mLastChar == ':') //consume the '::'
        {
            mState.IdentifierStr += ':';
            nextChar();
        }
        nextChar();
        while(isalnum(mLastChar) || mLastChar == '_' || (mLastChar == ':' && peekChar() == ':')) //[0-9a-zA-Z_]
        {
            mState.IdentifierStr.push_back(mLastChar);
            if(mLastChar == ':') //consume the '::'
            {
                mState.IdentifierStr += ':';
                nextChar();
            }
            nextChar();
        }

        //keywords
        auto found = mKeywordMap.find(mState.IdentifierStr);
        if(found != mKeywordMap.end())
            return found->second;

        return tok_identifier;
    }

    //hex numbers
    if(mLastChar == '0' && peekChar() == 'x') //0x
    {
        tokenLineIndex = mState.LineIndex - 1;
        nextChar(); //consume the 'x'
        mNumStr.clear();

        while (isxdigit(nextChar())) //[0-9a-fA-F]*
            mNumStr.push_back(mLastChar);

        if(!mNumStr.length()) //check for error condition
            return reportError("no hex digits after \"0x\" prefix");

        auto error = convertNumber(mNumStr.c_str(), mState.NumberVal, 16);
        if(error)
            return reportError(StringUtils::sprintf("convertNumber failed (%s) on hexadecimal number", error));
        mState.IsHexNumber = true;
        return tok_number;
    }

    if(isdigit(mLastChar)) //[0-9]
    {
        tokenLineIndex = mState.LineIndex - 1;
        mNumStr = mLastChar;

        while(isdigit(nextChar())) //[0-9]*
            mNumStr += mLastChar;

        auto error = convertNumber(mNumStr.c_str(), mState.NumberVal, 10);
        if(error)
            return reportError(StringUtils::sprintf("convertNumber failed (%s) on decimal number", error));
        mState.IsHexNumber = false;
        return tok_number;
    }

    //comments
    if(mLastChar == '/' && peekChar() == '/') //line comment
    {
        do
        {
            if(mLastChar == '\n')
                signalNewLine();
            nextChar();
        }
        while(!(mLastChar == EOF || mLastChar == '\n'));

        return getToken(tokenLineIndex); //interpret the next line
    }

    if(mLastChar == '/' && peekChar() == '*') //block comment
    {
        do
        {
            if(mLastChar == '\n')
                signalNewLine();
            nextChar();
        }
        while(!(mLastChar == EOF || mLastChar == '*' && peekChar() == '/'));

        if(mLastChar == EOF) //unexpected end of file
        {
            mState.LineIndex++;
            return reportError("unexpected end of file in block comment");
        }

        nextChar();
        nextChar();
        return getToken(tokenLineIndex); //get the next non-comment token
    }

    tokenLineIndex = mState.LineIndex - 1;

    //operators
    auto opFound = mOpTripleMap.find(MAKE_OP_TRIPLE(mLastChar, peekChar(), peekChar(1)));
    if(opFound != mOpTripleMap.end())
    {
        nextChar();
        nextChar();
        nextChar();
        return opFound->second;
    }
    opFound = mOpDoubleMap.find(MAKE_OP_DOUBLE(mLastChar, peekChar()));
    if(opFound != mOpDoubleMap.end())
    {
        nextChar();
        nextChar();
        return opFound->second;
    }
    opFound = mOpSingleMap.find(MAKE_OP_SINGLE(mLastChar));
    if(opFound != mOpSingleMap.end())
    {
        nextChar();
        return opFound->second;
    }

    //end of file
    if(mLastChar == EOF)
    {
        tokenLineIndex = 0;
        return tok_eof;
    }

    //unknown character
    return reportError(StringUtils::sprintf("unexpected character \'%c\'", mLastChar));
}

Lexer::Token Lexer::reportError(const std::string & error)
{
    mState.ErrorMessage = error;
    return tok_error;
}

int Lexer::nextChar()
{
    return mLastChar = readChar();
}

void Lexer::reportWarning(const std::string & warning)
{
    mWarnings.push_back(warning);
}

void Lexer::resetLexerState()
{
    mInput.clear();
    mInput.reserve(1024 * 1024);
    mIndex = 0;
    mWarnings.clear();
    clearReserve(mState.IdentifierStr);
    clearReserve(mState.StringLit);
    clearReserve(mNumStr, 16);
    mLastChar = ' ';
    mState.Clear();
}

std::unordered_map<std::string, Lexer::Token> Lexer::mKeywordMap;
std::unordered_map<Lexer::Token, std::string> Lexer::mReverseTokenMap;
std::unordered_map<int, Lexer::Token> Lexer::mOpTripleMap;
std::unordered_map<int, Lexer::Token> Lexer::mOpDoubleMap;
std::unordered_map<int, Lexer::Token> Lexer::mOpSingleMap;

void Lexer::setupTokenMaps()
{
    //setup keyword map
#define DEF_KEYWORD(keyword) mKeywordMap[#keyword] = tok_##keyword;
#include "keywords.h"
#undef DEF_KEYWORD

    //setup token maps
#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3) mOpTripleMap[MAKE_OP_TRIPLE(ch1, ch2, ch3)] = tok_##enumval;
#define DEF_OP_DOUBLE(enumval, ch1, ch2) mOpDoubleMap[MAKE_OP_DOUBLE(ch1, ch2)] = tok_##enumval;
#define DEF_OP_SINGLE(enumval, ch1) mOpSingleMap[MAKE_OP_SINGLE(ch1)] = tok_##enumval;
#include "operators.h"
#undef DEF_OP_TRIPLE
#undef DEF_OP_DOUBLE
#undef DEF_OP_SINGLE

    //setup reverse token maps
#define DEF_KEYWORD(keyword) mReverseTokenMap[tok_##keyword] = #keyword;
#include "keywords.h"
#undef DEF_KEYWORD

#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3) mReverseTokenMap[tok_##enumval] = std::string({ch1, ch2, ch3});
#define DEF_OP_DOUBLE(enumval, ch1, ch2) mReverseTokenMap[tok_##enumval] = std::string({ch1, ch2});
#define DEF_OP_SINGLE(enumval, ch1) mReverseTokenMap[tok_##enumval] = std::string({ch1});
#include "operators.h"
#undef DEF_OP_TRIPLE
#undef DEF_OP_DOUBLE
#undef DEF_OP_SINGLE
}

std::string Lexer::TokString(const TokenState & ts)
{
    switch(ts.Token)
    {
    case tok_eof:
        return "tok_eof";
    case tok_error:
        return StringUtils::sprintf("error(line %d, col %d, \"%s\")", ts.CurLine + 1, ts.LineIndex, ts.ErrorMessage.c_str());
    case tok_identifier:
        return ts.IdentifierStr;
    case tok_number:
        return StringUtils::sprintf(ts.IsHexNumber ? "0x%llX" : "%llu", ts.NumberVal);
    case tok_stringlit:
        return StringUtils::sprintf("\"%s\"", StringUtils::Escape(ts.StringLit).c_str());
    case tok_charlit:
    {
        std::string s;
        s = ts.CharLit;
        return StringUtils::sprintf("'%s'", StringUtils::Escape(s).c_str());
    }
    default:
    {
        auto found = mReverseTokenMap.find(ts.Token);
        if(found != mReverseTokenMap.end())
            return found->second;
        return "<UNKNOWN TOKEN>";
    }
    }
}

std::string Lexer::TokString(Token tok)
{
    switch(tok)
    {
    case tok_eof:
        return "tok_eof";
    case tok_error:
        return StringUtils::sprintf("error(line %d, col %d, \"%s\")", mState.CurLine + 1, mState.LineIndex, mState.ErrorMessage.c_str());
    case tok_identifier:
        return mState.IdentifierStr;
    case tok_number:
        return StringUtils::sprintf(mState.IsHexNumber ? "0x%llX" : "%llu", mState.NumberVal);
    case tok_stringlit:
        return StringUtils::sprintf("\"%s\"", StringUtils::Escape(mState.StringLit).c_str());
    case tok_charlit:
    {
        std::string s;
        s = mState.CharLit;
        return StringUtils::sprintf("'%s'", StringUtils::Escape(s).c_str());
    }
    default:
    {
        auto found = mReverseTokenMap.find(Token(tok));
        if(found != mReverseTokenMap.end())
            return found->second;
        return "<UNKNOWN TOKEN>";
    }
    }
}

int Lexer::peekChar(size_t distance)
{
    if(mIndex + distance >= mInput.size())
        return EOF;
    auto ch = mInput[mIndex + distance];
    if(ch == '\0')
    {
        reportWarning(StringUtils::sprintf("\\0 character in file data"));
        return peekChar(distance + 1);
    }
    return ch;
}

int Lexer::readChar()
{
    if(mIndex == mInput.size())
        return EOF;
    auto ch = mInput[mIndex++];
    mState.LineIndex++;
    if(ch == '\0')
    {
        reportWarning(StringUtils::sprintf("\\0 character in file data"));
        return readChar();
    }
    return ch;
}

bool Lexer::checkString(const std::string & expected)
{
    for(size_t i = 0; i < expected.size(); i++)
    {
        auto ch = peekChar(i);
        if(ch == EOF)
            return false;
        if(ch != uint8_t(expected[i]))
            return false;
    }
    mIndex += expected.size();
    return true;
}

void Lexer::signalNewLine()
{
    mState.CurLine++;
    mState.LineIndex = 0;
}
