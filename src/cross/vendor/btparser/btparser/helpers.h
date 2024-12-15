#pragma once

#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#endif // _WIN32

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__clang__)
#define __debugbreak() __builtin_debugtrap()
#elif defined(__GNUC__)
#define __debugbreak() __builtin_trap()
#else
#warning Unsupported platform/compiler
#include <signal.h>
#define __debugbreak() raise(SIGTRAP)
#endif // _MSC_VER

#ifndef _MSC_VER
template<size_t Count, typename... Args>
static int sprintf_s(char (&dst)[Count], const char* fmt, Args&&... args)
{
    return snprintf(dst, Count, fmt, std::forward<Args>(args)...);
}
#endif // _MSC_VER

namespace StringUtils
{
    inline std::string sprintf(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        std::vector<char> buffer(256, '\0');
        while(true)
        {
#ifdef _WIN32
            int res = _vsnprintf_s(buffer.data(), buffer.size(), _TRUNCATE, format, args);
#else
            int res = vsnprintf(buffer.data(), buffer.size(), format, args);
#endif // _WIN32
            if(res == -1)
            {
                buffer.resize(buffer.size() * 2);
                continue;
            }
            else
                break;
        }
        va_end(args);
        return std::string(buffer.data());
    }

    inline std::string Escape(const std::string & s)
    {
        auto escape = [](unsigned char ch) -> std::string
        {
            char buf[8] = "";
            switch(ch)
            {
            case '\0':
                return "\\0";
            case '\t':
                return "\\t";
            case '\f':
                return "\\f";
            case '\v':
                return "\\v";
            case '\n':
                return "\\n";
            case '\r':
                return "\\r";
            case '\\':
                return "\\\\";
            case '\"':
                return "\\\"";
            default:
                if(!isprint(ch)) //unknown unprintable character
                {
                    sprintf_s(buf, "\\x%02X", ch);
                }
                else
                    *buf = ch;
                return buf;
            }
        };
        std::string escaped;
        escaped.reserve(s.length() + s.length() / 2);
        for(size_t i = 0; i < s.length(); i++)
            escaped.append(escape((unsigned char)s[i]));
        return escaped;
    }

#ifdef _WIN32
    inline std::wstring Utf8ToUtf16(const std::string & str)
    {
        std::wstring convertedString;
        int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if(requiredSize > 0)
        {
            std::vector<wchar_t> buffer(requiredSize);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], requiredSize);
            convertedString.assign(buffer.begin(), buffer.end() - 1);
        }
        return convertedString;
    }
#endif // _WIN32

    inline void Split(const std::string& s, char delim, std::vector<std::string>& elems)
    {
        elems.clear();
        std::string item;
        item.reserve(s.length());
        for (size_t i = 0; i < s.length(); i++)
        {
            if (s[i] == delim)
            {
                if (!item.empty())
                    elems.push_back(item);
                item.clear();
            }
            else
                item.push_back(s[i]);
        }
        if (!item.empty())
            elems.push_back(std::move(item));
    }

    inline std::vector<std::string> Split(const std::string& s, char delim)
    {
        std::vector<std::string> elems;
        Split(s, delim, elems);
        return elems;
    }
};

#include <fstream>

namespace FileHelper
{
    inline bool ReadAllData(const std::string & fileName, std::vector<uint8_t> & content)
    {
        std::ifstream file(fileName, std::ios::binary);

        if (!file.is_open())
        {
            return false;
        }

        // Get the size of the file
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // Resize the vector to fit the entire file
        content.resize(static_cast<size_t>(fileSize));

        // Read the file into the vector
        file.read(reinterpret_cast<char*>(&content[0]), fileSize);
        return !file.bad();
    }

    inline bool WriteAllData(const std::string & fileName, const void* data, size_t size)
    {
        std::ofstream file(fileName, std::ios::binary);

        if (!file.is_open())
        {
            return false;
        }

        // Write the data to the file
        file.write(static_cast<const char*>(data), size);
        return !file.bad();
    }

    inline bool ReadAllText(const std::string & fileName, std::string & content)
    {
        std::vector<unsigned char> data;
        if(!ReadAllData(fileName, data))
            return false;
        data.push_back(0);
        content = std::string((const char*)data.data());
        return true;
    }

    inline bool WriteAllText(const std::string & fileName, const std::string & content)
    {
        return WriteAllData(fileName, content.c_str(), content.length());
    }
};