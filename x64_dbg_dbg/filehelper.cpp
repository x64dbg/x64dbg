#include "filehelper.h"

bool FileHelper::ReadAllText(const String & fileName, String & content)
{
    Handle hFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if(hFile == INVALID_HANDLE_VALUE)
        return false;
    unsigned int filesize = GetFileSize(hFile, 0);
    if(!filesize)
    {
        content.clear();
        return true;
    }
    Memory<char*> filedata(filesize + 1, "FileReader::ReadAllText:filedata");
    DWORD read = 0;
    if(!ReadFile(hFile, filedata(), filesize, &read, 0))
        return false;
    content = String(filedata());
    return true;
}

bool FileHelper::WriteAllText(const String & fileName, const String & content)
{
    Handle hFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if(hFile == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    return !!WriteFile(hFile, content.c_str(), DWORD(content.length()), &written, nullptr);
}