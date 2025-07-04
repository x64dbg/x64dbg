#pragma once

#include <QString>
#include <string>

inline QString formatHex(uint64_t value)
{
    return QString("%1").arg(value, 16, 16, QChar('0'));
}

#ifndef WINDOWS
// Protection
#define PAGE_EXECUTE 0x10
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_NOACCESS 0x1
#define PAGE_READONLY 0x2
#define PAGE_READWRITE 0x4
#define PAGE_WRITECOPY 0x8
// Only these can be combined
#define PAGE_GUARD 0x100
#define PAGE_NOCACHE 0x200
#define PAGE_WRITECOMBINE 0x400

// Type
#define MEM_IMAGE 0x1000000
#define MEM_MAPPED 0x40000
#define MEM_PRIVATE 0x20000

// State
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define MEM_FREE 0x10000
#endif // WINDOWS

inline std::string ProtectToString(uint32_t protect)
{
    auto result = [](uint32_t baseProtect) -> std::string
    {
        switch(baseProtect)
        {
        case PAGE_EXECUTE:
            return "PAGE_EXECUTE";
        case PAGE_EXECUTE_READ:
            return "PAGE_EXECUTE_READ";
        case PAGE_EXECUTE_READWRITE:
            return "PAGE_EXECUTE_READWRITE";
        case PAGE_EXECUTE_WRITECOPY:
            return "PAGE_EXECUTE_WRITECOPY";
        case PAGE_NOACCESS:
            return "PAGE_NOACCESS";
        case PAGE_READONLY:
            return "PAGE_READONLY";
        case PAGE_READWRITE:
            return "PAGE_READWRITE";
        case PAGE_WRITECOPY:
            return "PAGE_WRITECOPY";
        case 0:
            return "";
        default:
        {
            char res[16];
            snprintf(res, sizeof(res), "%X", baseProtect);
            return res;
        }
        }
    }(protect & 0xFF);

    switch(protect & 0xF00)
    {
    case PAGE_GUARD:
        result += "|PAGE_GUARD";
        break;
    case PAGE_NOCACHE:
        result += "|PAGE_NOCACHE";
        break;
    case PAGE_WRITECOMBINE:
        result += "|PAGE_WRITECOMBINE";
        break;
    default:
        break;
    }
    return result;
}

inline std::string ProtectToStringShort(uint32_t protect)
{
    auto result = [](uint32_t baseProtect) -> std::string
    {
        switch(baseProtect)
        {
        case PAGE_EXECUTE:
            return "E---";
        case PAGE_EXECUTE_READ:
            return "ER--";
        case PAGE_EXECUTE_READWRITE:
            return "ERW-";
        case PAGE_EXECUTE_WRITECOPY:
            return "E-WC";
        case PAGE_NOACCESS:
            return "----";
        case PAGE_READONLY:
            return "-R--";
        case PAGE_READWRITE:
            return "-RW-";
        case PAGE_WRITECOPY:
            return "--WC";
        case 0:
            return "----";
        default:
        {
            char res[16];
            snprintf(res, sizeof(res), "%-4X", baseProtect);
            return res;
        }
        }
    }(protect & 0xFF);

    switch(protect & 0xF00)
    {
    case PAGE_GUARD:
        result += "G";
        break;
    case PAGE_NOCACHE:
        result += "N";
        break;
    case PAGE_WRITECOMBINE:
        result += "C";
        break;
    default:
        result += "-";
        break;
    }
    return result;
}

inline std::string StateToString(const uint32_t State)
{
    switch(State)
    {
    case MEM_COMMIT:
    {
        return "MEM_COMMIT";
    }

    case MEM_RESERVE:
    {
        return "MEM_RESERVE";
    }

    case MEM_FREE:
    {
        return "MEM_FREE";
    }

    default:
    {
        char res[16];
        snprintf(res, sizeof(res), "%X", State);
        return res;
    }
    }
}

inline std::string StateToStringShort(const uint32_t State)
{
    switch(State)
    {
    case MEM_COMMIT:
    {
        return "COMMIT";
    }

    case MEM_RESERVE:
    {
        return "RESERVE";
    }

    case MEM_FREE:
    {
        return "FREE";
    }

    default:
    {
        char res[16];
        snprintf(res, sizeof(res), "%X", State);
        return res;
    }
    }
}

inline std::string TypeToString(const uint32_t Type)
{
    switch(Type)
    {
    case MEM_PRIVATE:
    {
        return "MEM_PRIVATE";
    }
    case MEM_MAPPED:
    {
        return "MEM_MAPPED";
    }
    case MEM_IMAGE:
    {
        return "MEM_IMAGE";
    }
    case 0:
        return "";

    default:
    {
        char res[16];
        snprintf(res, sizeof(res), "%X", Type);
        return res;
    }
    }
}

inline std::string TypeToStringShort(const uint32_t Type)
{
    switch(Type)
    {
    case MEM_PRIVATE:
    {
        return "PRV";
    }
    case MEM_MAPPED:
    {
        return "MAP";
    }
    case MEM_IMAGE:
    {
        return "IMG";
    }
    case 0:
        return "";

    default:
    {
        char res[16];
        snprintf(res, sizeof(res), "%X", Type);
        return res;
    }
    }
}
