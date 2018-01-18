#include "filehelper.h"
#include "handle.h"
#include "stringutils.h"

bool FileHelper::ReadAllData(const String & fileName, std::vector<unsigned char> & content)
{
    if(fileName.compare("x64dbg://localhost/clipboard") != 0)
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
    else
    {
        if(!OpenClipboard(0))
            return false;
        HANDLE hData;
        hData = GetClipboardData(CF_UNICODETEXT);
        if(hData == NULL)
        {
            CloseClipboard();
            return false;
        }
        wchar_t* wideString = reinterpret_cast<wchar_t*>(GlobalLock(hData));
        if(wideString == nullptr)
        {
            CloseClipboard();
            return false;
        }
        String text = StringUtils::Utf16ToUtf8(wideString);
        GlobalUnlock(hData);
        CloseClipboard();
        content.resize(text.size());
        memcpy(content.data(), text.c_str(), text.size());
        return true;
    }
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

bool FileHelper::ReadAllLines(const String & fileName, std::vector<String> & lines, bool keepEmpty)
{
    String content;
    if(!ReadAllText(fileName, content))
        return false;
    lines.clear();
    String line;
    for(auto ch : content)
    {
        switch(ch)
        {
        case '\r':
            break;
        case '\n':
            if(line.length() || keepEmpty)
                lines.push_back(line);
            line.clear();
            break;
        default:
            line.resize(line.length() + 1);
            line[line.length() - 1] = ch;
            break;
        }
    }
    if(line.length())
        lines.push_back(line);
    return true;
}

String FileHelper::GetFileName(const String & fileName)
{
    auto last = strrchr(fileName.c_str(), '\\');
    return last ? last + 1 : fileName;
}
