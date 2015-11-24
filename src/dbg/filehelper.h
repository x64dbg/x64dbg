#ifndef _FILEREADER_H
#define _FILEREADER_H

#include "_global.h"

class FileHelper
{
public:
    static bool ReadAllText(const String & fileName, String & content);
    static bool WriteAllText(const String & fileName, const String & content);
};

#endif //_FILEREADER_H