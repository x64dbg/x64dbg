#include "patternfind.h"
#include <vector>
#include <algorithm>

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

    //reject patterns with unsupported charcters
    for(char ch : patterntext)
        if(ch != '?' && ch != ' ' && !isHex(ch))
            return false;

    string formattext = formathexpattern(patterntext);
    int len = (int)formattext.length();
    if(!len)
        return false;

    if(len % 2) //not a multiple of 2
    {
        formattext += '?';
        len++;
    }

    PatternByte newByte = {};
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

    //reject wildcard only patterns
    bool allWildcard = std::all_of(pattern.begin(), pattern.end(), [](const PatternByte & patternByte)
    {
        return patternByte.nibble[0].wildcard & patternByte.nibble[1].wildcard;
    });
    if(allWildcard)
        return false;

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

static bool isByteFullySpecified(const PatternByte & pbyte)
{
    return !pbyte.nibble[0].wildcard && !pbyte.nibble[1].wildcard;
}

static unsigned char getByteValue(const PatternByte & pbyte)
{
    return (pbyte.nibble[0].data << 4) | pbyte.nibble[1].data;
}

// Boyer-Moore-Horspool pattern search with wildcard support
size_t patternfind(const unsigned char* data, size_t datasize, const std::vector<PatternByte> & pattern)
{
    size_t searchpatternsize = pattern.size();

    if(searchpatternsize == 0 || datasize < searchpatternsize)
        return -1;

    // Find the rightmost fully-specified byte to use as anchor
    int anchorPos = -1;
    unsigned char anchorByte = 0;
    for(int i = (int)searchpatternsize - 1; i >= 0; i--)
    {
        if(isByteFullySpecified(pattern[i]))
        {
            anchorPos = i;
            anchorByte = getByteValue(pattern[i]);
            break;
        }
    }

    // If no fully-specified byte found, fall back to naive search
    if(anchorPos == -1)
    {
        // All bytes have wildcards - use naive search with early exit
        for(size_t i = 0; i <= datasize - searchpatternsize; i++)
        {
            bool match = true;
            for(size_t j = 0; j < searchpatternsize; j++)
            {
                if(!patternmatchbyte(data[i + j], pattern[j]))
                {
                    match = false;
                    break;
                }
            }
            if(match)
                return i;
        }
        return -1;
    }

    // Build BMH skip table (256 entries)
    // skip[byte] = how many positions to shift when 'byte' is seen at anchor position
    size_t skip[256];
    size_t suffixLen = searchpatternsize - anchorPos - 1; // Bytes after anchor

    // Default: skip entire pattern length minus suffix
    size_t defaultSkip = searchpatternsize - suffixLen;
    for(int i = 0; i < 256; i++)
        skip[i] = defaultSkip;

    // For bytes that appear in the pattern before anchor, set smaller skip
    // We only consider fully-specified bytes for the skip table
    for(size_t i = 0; i < (size_t)anchorPos; i++)
    {
        if(isByteFullySpecified(pattern[i]))
        {
            unsigned char byte = getByteValue(pattern[i]);
            skip[byte] = anchorPos - i;
        }
    }

    // BMH search loop
    size_t pos = 0;
    while(pos <= datasize - searchpatternsize)
    {
        // Check anchor byte first (most likely to mismatch)
        if(data[pos + anchorPos] == anchorByte)
        {
            // Anchor matched - verify full pattern
            bool fullMatch = true;
            for(size_t i = 0; i < searchpatternsize; i++)
            {
                if(!patternmatchbyte(data[pos + i], pattern[i]))
                {
                    fullMatch = false;
                    break;
                }
            }

            if(fullMatch)
                return pos;

            // Mismatch after anchor - shift by 1 to avoid missing matches
            pos++;
        }
        else
        {
            // Anchor didn't match - use skip table to jump ahead
            unsigned char mismatchByte = data[pos + anchorPos];
            pos += skip[mismatchByte];
        }
    }

    return -1;
}