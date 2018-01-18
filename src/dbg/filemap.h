#pragma once

#include <windows.h>
#include <string>

template<typename T>
struct FileMap
{
    bool Map(const wchar_t* szFileName)
    {
        hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            size = GetFileSize(hFile, nullptr);
            hMap = CreateFileMappingW(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
            if(hMap)
                data = (const T*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        }
        return data != nullptr;
    }

    unsigned int Size()
    {
        return size;
    }

    const T* Data()
    {
        return data;
    }

    void Unmap()
    {
        if(data)
            UnmapViewOfFile(data);
        if(hMap)
            CloseHandle(hMap);
        if(hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);

        hFile = INVALID_HANDLE_VALUE;
        hMap = nullptr;
        data = nullptr;
        size = 0;
    }

    ~FileMap()
    {
        Unmap();
    }

private:
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = nullptr;
    const T* data = nullptr;
    unsigned int size = 0;
};

struct BufferedWriter
{
    explicit BufferedWriter(HANDLE hFile, size_t size = 65536)
        : hFile(hFile),
          mBuffer(new char[size]),
          mSize(size),
          mIndex(0)
    {
        memset(mBuffer, 0, size);
    }

    bool Write(const void* buffer, size_t size)
    {
        for(size_t i = 0; i < size; i++)
        {
            mBuffer[mIndex++] = ((const char*)buffer)[i];
            if(mIndex == mSize)
            {
                if(!flush())
                    return false;
                mIndex = 0;
            }
        }
        return true;
    }

    ~BufferedWriter()
    {
        flush();
        delete[] mBuffer;
        CloseHandle(hFile);
    }

private:
    HANDLE hFile;
    char* mBuffer;
    size_t mSize;
    size_t mIndex;

    bool flush()
    {
        if(!mIndex)
            return true;
        DWORD written;
        auto result = WriteFile(hFile, mBuffer, DWORD(mIndex), &written, nullptr);
        mIndex = 0;
        return !!result;
    }
};