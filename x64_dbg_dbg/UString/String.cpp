/**
 * UTF8 string library.
 *
 * Allows to use native UTF8 sequences as a string class. Has many overloaded
 * operators that provides such features as concatenation, types converting and
 * much more.
 *
 * Distributed under GPL v3
 *
 * Author:
 *      Grigory Gorelov (gorelov@grigory.info)
 *      See more information on grigory.info
 */

#include "String.h"
#include <string>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <ostream>
#include <stdint.h>
#include <errno.h>
#include "Exception.h"

bool UTF8::String::HasThisString(const UTF8::String & Str) const
{
    return GetSubstringPosition(Str) != -1;
}

bool UTF8::String::CharacterIsOneOfThese(const UTF8::String & Characters) const
{
    if(Length() == 1)
    {
        for(unsigned int i = 0; i < Characters.Length(); i++)
        {
            if(Characters[i] == *this)
            {
                return true;
            }
        }
        return false;
    }
    else
    {
        throw Exception("[CharacterIsOneOfThese] String is more then one character length: \"" + ToString() + "\"", UTF8::Exception::StringIsNotACharacter);
    }
}

UTF8::String UTF8::String::FromFile(const UTF8::String & Path)
{
    UTF8::String s;

    std::ifstream File;
    File.open(Path.ToConstCharPtr());

    if(File.is_open())
    {
        File.seekg(0, std::ios::end);
        size_t Length = (size_t)File.tellg();
        File.seekg(0, std::ios::beg);

        char* buf = new char[Length + 1];
        memset(buf, 0, Length + 1);

        File.read(buf, Length);
        s.AppendString(buf);

        delete[] buf;
    }
    else
    {
        throw Exception("Cannot open file \"" + Path.ToString() + "\"", UTF8::Exception::FileNotFound);
    }

    File.close();

    return s;
}

long UTF8::String::Search(const UTF8::String & SubString, unsigned int StartPosition, int Direction) const
{
    unsigned int SubstringLength = SubString.Length();
    unsigned int n = StartPosition;

    if(n > Length() - SubstringLength)
    {
        if(Direction == SearchDirectionFromLeftToRight)
        {
            return -1;
        }
        else
        {
            n = Length() - SubstringLength;
        }
    }

    if((int)n < 0)
    {
        if(Direction == SearchDirectionFromRightToLeft)
        {
            return -1;
        }
        else
        {
            n = 0;
        }
    }

    while(((Direction == SearchDirectionFromLeftToRight) && (n < Length() - SubstringLength + 1)) || ((Direction == SearchDirectionFromRightToLeft) && ((int)n >= 0)))
    {

        if(this->Substring(n, SubstringLength) == SubString)
        {
            return n;
        }

        n += Direction == SearchDirectionFromLeftToRight ? 1 : -1;
    }

    return -1;
}

std::ostream & operator<<(std::ostream & os, const UTF8::String & s)
{
    os << s.ToString();

    return os;
}

bool operator==(const char* str, const UTF8::String & StringObj)
{
    return StringObj == str;
}

bool operator==(const std::string & str, const UTF8::String & StringObj)
{
    return StringObj == str;
}

bool operator!=(const char* str, const UTF8::String & StringObj)
{
    return StringObj != str;
}

bool operator!=(const std::string & str, const UTF8::String & StringObj)
{
    return StringObj != str;
}

UTF8::String UTF8::String::Quote() const
{
    return "\"" + (*this) + "\"";
}

UTF8::String UTF8::String::Trim() const
{
    UTF8::String result = *this;
    unsigned int i = 0;

    while((result[i] == " ") || (result[i] == "\n") || (result[i] == "\r") || (result[i] == "\t"))
    {
        i++;
    }

    if(i == result.Length())
    {
        return UTF8::String();
    }

    long j = result.Length();
    while((result[j - 1] == " ") || (result[j - 1] == "\n") || (result[j - 1] == "\r") || (result[j - 1] == "\t"))
    {
        j--;
    }

    result = result.Substring(i, j - i);

    return result;
}

UTF8::String UTF8::String::Replace(const UTF8::String & Search, const UTF8::String & Replace) const
{
    UTF8::String result = *this;

    // Long to cover unsigned int and -1
    long pos = 0;
    while((pos = result.Search(Search, pos)) != -1)
    {
        result = result.SubstringReplace(pos, Search.Length(), Replace);

        // Next time we search after replacement
        pos += Replace.Length();
    }

    return result;

}

UTF8::String UTF8::String::SubstringReplace(unsigned int Start, unsigned int Count, const UTF8::String & Replace) const
{
    if(Start < Length())
    {
        return (Start ? Substring(0, Start) : UTF8::String()) + Replace + Substring(Start + Count);
    }
    else
    {
        return *this;
    }
}

UTF8::String UTF8::String::Implode(const std::vector <UTF8::String> & Strings, const UTF8::String & Separator)
{
    if(Strings.size())
    {
        UTF8::String Result;

        for(unsigned int i = 0; i < Strings.size(); i++)
        {
            if(Result.Length())
            {
                Result += Separator;
            }
            Result += Strings[i];
        }
        return Result;
    }
    else
    {
        return UTF8::String();
    }
}

std::vector <UTF8::String> UTF8::String::Explode(const String & Separator) const
{
    std::vector <UTF8::String> v;

    unsigned int prev = 0;

    unsigned int i = 0;

    while(i < Length() - Separator.Length() + 1)
    {
        if(Substring(i, Separator.Length()) == Separator)
        {
            if(i - prev > 0)
            {
                v.push_back(Substring(prev, i - prev));
            }
            i += Separator.Length();
            prev = i;
        }
        else
        {
            i++;
        }
    }

    if(prev < Length())
    {
        v.push_back(Substring(prev, Length() - prev));
    }

    return v;
}

UTF8::String operator+(const char* CharPtr, const UTF8::String & StringObj)
{
    UTF8::String s(CharPtr);
    s += StringObj;

    return s;
}

UTF8::String operator+(const std::string & str, const UTF8::String & StringObj)
{
    UTF8::String s(str);
    s += StringObj;

    return s;
}

UTF8::String UTF8::String::operator+(const UTF8::String & s) const
{
    UTF8::String res(*this);
    res.AppendString(s.Data);

    return res;
}

UTF8::String & UTF8::String::operator+=(const UTF8::String & s)
{
    AppendString(s.Data);

    return *this;
}

void UTF8::String::AppendString(const char* str)
{
    // The functions that can fill buffer directly:
    //
    //       SetString         AppendString
    //
    // Make sure all preparations are done there

    if(str && strlen(str))
    {
        if(DataArrayLength)
        {
            CheckIfStringIsCorrect(str);

            unsigned int StrLength = (unsigned int)strlen(str);

            Data = (char*) realloc(Data, DataArrayLength + StrLength + 1);

            if(Data != NULL)
            {
                memcpy(Data + DataArrayLength, str, StrLength);
                DataArrayLength += StrLength;
                Data[DataArrayLength] = 0;

                CalculateStringLength();
            }
            else
            {
                throw Exception("[AppendString] Cannot realloc any more memory");
            }
        }
        else
        {
            SetString(str);
        }
    }
}

void UTF8::String::SetString(const char* str)
{
    // The functions that can fill buffer directly:
    //
    //       SetString         AppendString
    //
    // Make sure all preparations are done there

    if(str && strlen(str))
    {
        CheckIfStringIsCorrect(str);

        Empty();

        DataArrayLength = (unsigned int)strlen(str);
        Data = new char[DataArrayLength + 1];
        Data[DataArrayLength] = 0;

        memcpy(Data, str, DataArrayLength);

        CalculateStringLength();
    }
    else
    {
        Empty();
    }
}

void UTF8::String::ConvertFromInt64(int64_t n)
{
    Empty();

    if(n)
    {
        bool minus;
        if(n < 0)
        {
            n = -n;
            minus = true;
        }
        else
        {
            minus = false;
        }

        char tmp[32] = "0";
        const char* num = "0123456789";
        memset(tmp, 0, 32);

        unsigned int i = 30;

        while(n)
        {
            tmp[i] = num[n % 10];
            n /= 10;
            i--;

            if(((int)i < 0) || ((i < 1) && minus))
            {
                throw Exception("[ConvertFromInt] Cycle terminated, buffer overflow.");
            }
        }

        if(minus)
        {
            tmp[i] = '-';
            i--;
        }

        SetString(tmp + i + 1);
    }
    else
    {
        SetString("0");
    }

    CalculateStringLength();
}

void UTF8::String::InitString()
{
    Data = NULL;
    DataArrayLength = 0;
    StringLength = 0;
}

UTF8::String::String()
{
    InitString();
}

UTF8::String::String(const std::string & s)
{
    InitString();
    CheckIfStringIsCorrect(s.c_str());
    AppendString(s.c_str());
    CalculateStringLength();
}

int UTF8::String::GetSymbolIndexInDataArray(unsigned int Position) const
{
    if(Position >= StringLength)
    {
        throw Exception(UTF8::String("[GetSymbolIndexInDataArray] trying to get position beyond the end of string."));
    }

    unsigned int n = 0;
    for(unsigned int i = 0; i < Position; i++)
    {
        n += GetSequenceLength(Data + n);
    }
    return n;

}

long UTF8::String::GetSubstringPosition(const UTF8::String & SubString, unsigned int Start) const
{
    if(SubString.Length() > StringLength)
    {
        return -1;
    }

    unsigned int ScansCount = StringLength - SubString.StringLength + 1 - Start;
    for(unsigned int i = 0; i < ScansCount; i++)
    {
        if(this->Substring(i + Start, SubString.StringLength) == SubString)
        {
            return i + Start;
        }
    }

    return -1;
}

UTF8::String UTF8::String::Substring(unsigned int Start, unsigned int Count) const
{
    if(Start >= StringLength)
    {
        return UTF8::String();
    }

    if((Start + Count > StringLength) || (Count == 0))
    {
        Count = StringLength - Start;
    }

    unsigned int StartIndex = GetSymbolIndexInDataArray(Start);
    unsigned int CopyAmount = 0;

    for(unsigned int i = 0; i < Count; i++)
    {
        CopyAmount += GetSequenceLength(Data + StartIndex + CopyAmount);
    }

    char* tmp = new char[CopyAmount + 1];
    memcpy(tmp, Data + StartIndex, CopyAmount);
    tmp[CopyAmount] = 0;

    UTF8::String r(tmp);
    delete[] tmp;

    return r;
}

UTF8::String::String(const char* str)
{
    InitString();
    SetString(str);
}

UTF8::String::String(const uint32_t* str)
{
    InitString();
    ConvertFromUTF32(str);
}

void UTF8::String::ConvertFromUTF32(const uint32_t* s)
{
    if(s)
    {
        unsigned int WideStringLength = 0;
        do
        {
            WideStringLength++;
            if(WideStringLength == 4294967295UL)
            {
                throw Exception("[ConvertFromUTF32] Cannot find termination symbol in incoming string.");
            }
        }
        while(s[WideStringLength]);

        char* tmp = new char[WideStringLength * 4 + 1];
        memset(tmp, 0, WideStringLength * 4 + 1);
        unsigned int pos = 0;

        for(unsigned int i = 0; i < WideStringLength; i++)
        {
            uint32_t wc = s[i];

            if(wc < 0x80)
            {
                tmp[pos++] = wc;
            }
            else if(wc < 0x800)
            {
                tmp[pos++] = (wc >> 6) | 0xC0 /* 0b11000000 */;
                tmp[pos++] = (wc & 0x3F /* 0b00111111 */) | 0x80 /* 0b10000000 */;
            }
            else if(wc < 0x10000)
            {
                tmp[pos++] = (wc >> 12) | 0xE0 /* 0b11100000 */;
                tmp[pos++] = ((wc >> 6) & 0x3F /* 0b00111111 */) | 0x80 /* 0b10000000 */;
                tmp[pos++] = (wc & 0x3F /* 0b00111111 */) | 0x80 /* 0b10000000 */;
            }
            else
            {

                tmp[pos++] = (wc >> 18) | 0xF0 /* 0b11110000 */;
                tmp[pos++] = ((wc >> 12) & 0x3F /* 0b00111111 */) | 0x80 /* 0b10000000 */;
                tmp[pos++] = ((wc >> 6) & 0x3F /* 0b00111111 */) | 0x80 /* 0b10000000 */;
                tmp[pos++] = (wc & 0x3F /* 0b00111111 */) | 0x80 /* 0b10000000 */;
            }
        }

        SetString(tmp);

        delete[] tmp;
    }
}

void UTF8::String::CalculateStringLength()
{
    // We are not writing anything to memory so limits are not needed
    if(Data)
    {
        unsigned int n = 0, count = 0;
        do
        {
            // We do not need to check line end here, it is checked when string is changed
            n += GetSequenceLength(Data + n);
            count++;
        }
        while(Data[n]);

        StringLength = count;
    }
    else
    {
        StringLength = 0;
    }
}

void UTF8::String::CheckIfStringIsCorrect(const char* str) const
{
    if(str)
    {
        // We are not writing anything to memory so limits are not needed
        unsigned int n = 0, i;
        unsigned int SequenceLength;
        while(str[n])
        {
            SequenceLength = GetSequenceLength(str + n);
            for(i = 1; i < SequenceLength; i++)
            {
                if((((unsigned char) str[n + i]) >> 6) != 0x2 /* 0b00000010 */)
                {
                    std::string s(str);
                    throw Exception("[CheckIfStringIsCorrect] Incorrect byte in UTF8 sequence: \"" + s + "\"");
                }
            }
            n += SequenceLength;
            if(n >= 0xFFFFFFFF - 4)
            {

                std::string s(str);
                throw Exception("[CheckIfStringIsCorrect] termination char was not found in string: \"" + s + "\"");
            }
        }
    }
}

bool UTF8::String::operator>(const UTF8::String & s) const
{
    if(*this == s)
    {
        return false;
    }

    if(*this < s)
    {
        return false;
    }

    return true;
}

bool UTF8::String::operator<(const UTF8::String & s) const
{
    unsigned int MinLength = StringLength < s.StringLength ? StringLength : s.StringLength;

    //std::cout << "MinLength=" << MinLength;

    unsigned int MyPos = 0, RemotePos = 0;
    unsigned int MySequenceLength, RemoteSequenceLength;
    for(unsigned int i = 0; i < MinLength; i++)
    {
        MySequenceLength = GetSequenceLength(Data + MyPos);
        RemoteSequenceLength = GetSequenceLength(s.Data + RemotePos);

        if(MySequenceLength < RemoteSequenceLength)
        {
            return true;
        }

        if(MySequenceLength > RemoteSequenceLength)
        {
            return false;
        }

        for(unsigned int j = 0; j < MySequenceLength; j++)
        {
            if(Data[MyPos + j] < s.Data[RemotePos + j])
            {
                return true;
            }

            if(Data[MyPos + j] > s.Data[RemotePos + j])
            {
                return false;
            }
        }

        MyPos += MySequenceLength;
        RemotePos += RemoteSequenceLength;
    }

    // If this string is substring of s (from left side) then it is lower
    return StringLength < s.StringLength;
}

UTF8::String UTF8::String::operator[](unsigned int const n) const
{
    if(n >= StringLength)
    {
        return UTF8::String();
    }

    if((int)n < 0)
    {
        return UTF8::String();
    }

    unsigned int pos = 0;
    for(unsigned int i = 0; i < n; i++)
    {
        pos += GetSequenceLength(Data + pos);
    }

    char t[5];
    memset(t, 0, 5);
    memcpy(t, Data + pos, GetSequenceLength(Data + pos));

    return UTF8::String(t);
}

unsigned int UTF8::String::GetSequenceLength(const char* StartByte) const
{
    if(StartByte && strlen(StartByte))
    {
        unsigned char Byte = StartByte[0];
        if(Byte < 128)
        {
            return 1;
        }

        // Here we need back order due to mask operation
        if((Byte >> 5) == 0x6 /* 0b00000110 */)
        {
            return 2;
        }

        if((Byte >> 4) == 0xE /* 0b00001110 */)
        {
            return 3;
        }

        if((Byte >> 3) == 0x1E /* 0b00011110 */)
        {
            return 4;
        }

        throw Exception(std::string("[GetSequenceLength] Invalid UTF8 start byte. My own string is: [") + Data + "] Argument is: [" + StartByte + "]");
    }
    else
    {
        if(StartByte == 0)
            StartByte = "(null)";
        throw Exception(std::string("[GetSequenceLength] Invalid UTF8 start byte (it is empty). My own string is: [") + Data + "] Argument is: [" + StartByte + "]");
    }
}

UTF8::String & UTF8::String::operator=(const String & Original)
{
    // Check if objects are not same
    if((unsigned int long) &Original != (unsigned int long) this)
    {
        Empty();
        SetString(Original.Data);
    }

    return *this;
}

UTF8::String & UTF8::String::operator=(const char* str)
{
    Empty();
    SetString(str);

    return *this;
}

UTF8::String & UTF8::String::operator=(const uint32_t* str)
{
    Empty();
    ConvertFromUTF32(str);

    return *this;
}

UTF8::String::operator std::string() const
{
    return this->ToString();
}

void UTF8::String::Empty()
{
    if(DataArrayLength)
    {
        delete Data;
        InitString();
    }
}

std::string UTF8::String::ToString() const
{
    if(DataArrayLength)
    {
        return std::string(Data);
    }
    else
    {
        return std::string();
    }
}

UTF8::String UTF8::String::operator+(const char* s) const
{
    UTF8::String res(*this);
    res.AppendString(s);

    return res;
}

bool UTF8::String::operator==(const UTF8::String & s) const
{
    if(DataArrayLength != s.DataArrayLength)
    {
        return false;
    }
    else
    {
        for(unsigned int i = 0; i < DataArrayLength; i++)
        {
            if(Data[i] != s.Data[i])
            {
                return false;
            }
        }
        return true;
    }
}

bool UTF8::String::operator!=(const UTF8::String & s) const
{
    return !(*this == s);
}

bool UTF8::String::operator==(const char* str) const
{
    if(str && strlen(str))
    {
        if(DataArrayLength != strlen(str))
        {
            return false;
        }
        else
        {
            for(unsigned int i = 0; i < DataArrayLength; i++)
            {
                if(Data[i] != str[i])
                {
                    return false;
                }
            }

            return true;
        }
    }
    else
    {
        return StringLength == 0;
    }
}

bool UTF8::String::operator!=(const char* str) const
{
    return !(*this == str);
}

const char* UTF8::String::ToConstCharPtr() const
{
    return Data;
}

const char* UTF8::String::c_str() const
{
    return Data;
}

unsigned int UTF8::String::Length() const
{
    return StringLength;
}

unsigned int UTF8::String::DataLength() const
{
    return DataArrayLength;
}

UTF8::String::~String()
{
    Empty();
}

UTF8::String::String(const String & orig)
{
    InitString();
    SetString(orig.Data);
}
