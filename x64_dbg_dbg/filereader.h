#ifndef _FILEREADER_H
#define _FILEREADER_H

#include "_global.h"

class FileReader
{
public:
    static bool ReadAllText(const String & fileName, String & content);
};

#endif //_FILEREADER_H