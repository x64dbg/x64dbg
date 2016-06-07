#include "filehelper.h"
#include "handle.h"
#include "stringutils.h"

bool FileHelper::ReadAllData(const String & fileName, std::vector<unsigned char> & content)
{
    Handle hFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if(hFile == INVALID_HANDLE_VALUE)
        return false;
    unsigned int filesize = GetFileSize(hFile, nullptr);
    if(!filesize)
    {
        content.clear();
        return true;
    }
    content.resize(filesize);
    DWORD read = 0;
    return !!ReadFile(hFile, content.data(), filesize, &read, nullptr);
}

bool FileHelper::WriteAllData(const String & fileName, const void* data, size_t size)
{
    Handle hFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if(hFile == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    return !!WriteFile(hFile, data, DWORD(size), &written, nullptr);
}

bool FileHelper::ReadAllText(const String & fileName, String & content)
{
    std::vector<unsigned char> data;
    if(!ReadAllData(fileName, data))
        return false;
    data.push_back(0);
    content = String((const char*)data.data());
    return true;
}

bool FileHelper::WriteAllText(const String & fileName, const String & content)
{
    return WriteAllData(fileName, content.c_str(), content.length());
}
