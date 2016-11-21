#ifndef _FILEREADER_H
#define _FILEREADER_H

#include "_global.h"

class FileHelper
{
public:
    static bool ReadAllData(const String & fileName, std::vector<unsigned char> & content);
    static bool WriteAllData(const String & fileName, const void* data, size_t size);
    static bool ReadAllText(const String & fileName, String & content);
    static bool WriteAllText(const String & fileName, const String & content);
    static bool ReadAllLines(const String & fileName, std::vector<String> & lines, bool keepEmpty = false);
    static String GetFileName(const String & fileName);
};

#endif //_FILEREADER_H