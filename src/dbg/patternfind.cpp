#include "patternfind.h"
#include <vector>

using namespace std;

static inline bool isHex(char ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

static inline string formathexpattern(const string & patterntext)
{
    string result;
    int len = (int)patterntext.length();
    for(int i = 0; i < len; i++)
        if(patterntext[i] == '?' || isHex(patterntext[i]))
            result += patterntext[i];
    return result;
}

static inline int hexchtoint(char ch)
{
    if(ch >= '0' && ch <= '9')
        return ch - '0';
    else if(ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else if(ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

bool patterntransform(const string & patterntext, vector<PatternByte> & pattern)
{
    pattern.clear();
    string formattext = formathexpattern(patterntext);
    int len = (int)formattext.length();
    if(!len)
        return false;

    if(len % 2) //not a multiple of 2
    {
        formattext += '?';
        len++;
    }

    PatternByte newByte;
    for(int i = 0, j = 0; i < len; i++)
    {
        if(formattext[i] == '?') //wildcard
        {
            newByte.nibble[j].wildcard = true; //match anything
        }
        else //hex
        {
            newByte.nibble[j].wildcard = false;
            newByte.nibble[j].data = hexchtoint(formattext[i]) & 0xF;
        }

        j++;
        if(j == 2) //two nibbles = one byte
        {
            j = 0;
            pattern.push_back(newByte);
        }
    }
    return true;
}

static inline bool patternmatchbyte(unsigned char byte, const PatternByte & pbyte)
{
    int matched = 0;

    unsigned char n1 = (byte >> 4) & 0xF;
    if(pbyte.nibble[0].wildcard)
        matched++;
    else if(pbyte.nibble[0].data == n1)
        matched++;

    unsigned char n2 = byte & 0xF;
    if(pbyte.nibble[1].wildcard)
        matched++;
    else if(pbyte.nibble[1].data == n2)
        matched++;

    return (matched == 2);
}

size_t patternfind(const unsigned char* data, size_t datasize, const char* pattern, int* patternsize)
{
    string patterntext(pattern);
    vector<PatternByte> searchpattern;
    if(!patterntransform(patterntext, searchpattern))
        return -1;
    return patternfind(data, datasize, searchpattern);
}

size_t patternfind(const unsigned char* data, size_t datasize, unsigned char* pattern, size_t patternsize)
{
    if(patternsize > datasize)
        patternsize = datasize;
    for(size_t i = 0, pos = 0; i < datasize; i++)
    {
        if(data[i] == pattern[pos])
        {
            pos++;
            if(pos == patternsize)
                return i - patternsize + 1;
        }
        else if(pos > 0)
        {
            i -= pos;
            pos = 0; //reset current pattern position
        }
    }
    return -1;
}

static inline void patternwritebyte(unsigned char* byte, const PatternByte & pbyte)
{
    unsigned char n1 = (*byte >> 4) & 0xF;
    unsigned char n2 = *byte & 0xF;
    if(!pbyte.nibble[0].wildcard)
        n1 = pbyte.nibble[0].data;
    if(!pbyte.nibble[1].wildcard)
        n2 = pbyte.nibble[1].data;
    *byte = ((n1 << 4) & 0xF0) | (n2 & 0xF);
}

void patternwrite(unsigned char* data, size_t datasize, const char* pattern)
{
    vector<PatternByte> writepattern;
    string patterntext(pattern);
    if(!patterntransform(patterntext, writepattern))
        return;
    size_t writepatternsize = writepattern.size();
    if(writepatternsize > datasize)
        writepatternsize = datasize;
    for(size_t i = 0; i < writepatternsize; i++)
        patternwritebyte(&data[i], writepattern.at(i));
}

bool patternsnr(unsigned char* data, size_t datasize, const char* searchpattern, const char* replacepattern)
{
    size_t found = patternfind(data, datasize, searchpattern);
    if(found == -1)
        return false;
    patternwrite(data + found, datasize - found, replacepattern);
    return true;
}

size_t patternfind(const unsigned char* data, size_t datasize, const std::vector<PatternByte> & pattern)
{
    size_t searchpatternsize = pattern.size();
    for(size_t i = 0, pos = 0; i < datasize; i++) //search for the pattern
    {
        if(patternmatchbyte(data[i], pattern.at(pos))) //check if our pattern matches the current byte
        {
            pos++;
            if(pos == searchpatternsize) //everything matched
                return i - searchpatternsize + 1;
        }
        else if(pos > 0) //fix by Computer_Angel
        {
            i -= pos;
            pos = 0; //reset current pattern position
        }
    }
    return -1;
}