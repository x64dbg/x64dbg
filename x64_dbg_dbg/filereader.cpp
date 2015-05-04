#include "filereader.h"

bool FileReader::ReadAllText(const String & fileName, String & content)
{
    Handle hFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile == INVALID_HANDLE_VALUE)
        return false;
    unsigned int filesize = GetFileSize(hFile, 0);
    if(!filesize)
    {
        content = "";
        return true;
    }
    Memory<char*> filedata(filesize + 1, "FileReader::ReadAllText:filedata");
    DWORD read = 0;
    if(!ReadFile(hFile, filedata, filesize, &read, 0))
        return false;
    content = String(filedata());
    return true;
}