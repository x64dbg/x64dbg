#pragma once

/*
MIT License

Copyright (c) 2018 Duncan Ogilvie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

struct IBufferedFileReader
{
    enum Direction
    {
        Right,
        Left
    };

    virtual ~IBufferedFileReader() {}
    virtual bool isopen() const = 0;
    virtual bool read(void* dest, uint64_t index, size_t size) = 0;
    virtual uint64_t size() = 0;
    virtual void setbuffersize(size_t size) = 0;
    virtual void setbufferdirection(Direction direction) = 0;

    bool readchar(uint64_t index, char & ch)
    {
        return read(&ch, index, 1);
    }

    bool readstring(uint64_t index, size_t size, std::string & str)
    {
        str.resize(size);
        return read((char*)str.c_str(), index, size);
    }

    bool readvector(uint64_t index, size_t size, std::vector<char> & vec)
    {
        vec.resize(size);
        return read(vec.data(), index, size);
    }
};

class HandleFileReader : public IBufferedFileReader
{
    HANDLE mHandle = INVALID_HANDLE_VALUE;
    uint64_t mFileSize = -1;

    std::vector<char> mBuffer;
    size_t mBufferIndex = 0;
    size_t mBufferSize = 0;
    Direction mBufferDirection = Right;

    bool readnobuffer(void* dest, uint64_t index, size_t size)
    {
        if(!isopen())
            return false;

        LARGE_INTEGER distance;
        distance.QuadPart = index;
        if(!SetFilePointerEx(mHandle, distance, nullptr, FILE_BEGIN))
            return false;

        DWORD read = 0;
        return !!ReadFile(mHandle, dest, (DWORD)size, &read, nullptr);
    }

public:
    HandleFileReader(const wchar_t* szFileName)
    {
        mHandle = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if(mHandle != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER size;
            if(GetFileSizeEx(mHandle, &size))
            {
                mFileSize = size.QuadPart;
            }
            else
            {
                CloseHandle(mHandle);
                mHandle = INVALID_HANDLE_VALUE;
            }
        }
    }

    ~HandleFileReader() override
    {
        if(isopen())
        {
            CloseHandle(mHandle);
            mHandle = INVALID_HANDLE_VALUE;
        }
    }

    HandleFileReader(const HandleFileReader &) = delete;

    bool isopen() const override
    {
        return mHandle != INVALID_HANDLE_VALUE;
    }

    bool read(void* dest, uint64_t index, size_t size) override
    {
        if(index + size > mFileSize)
            return false;

        if(size > mBufferSize)
            return readnobuffer(dest, index, size);

        if(index < mBufferIndex || index + size > mBufferIndex + mBuffer.size())
        {
            auto bufferSize = std::min(uint64_t(mBufferSize), mFileSize - index);
            mBuffer.resize(size_t(bufferSize));
            mBufferIndex = size_t(index);
            /*if (mBufferDirection == Left)
            {
                if (mBufferIndex >= mBufferSize + size)
                    mBufferIndex -= mBufferSize + size;
            }*/
            if(!readnobuffer(mBuffer.data(), mBufferIndex, mBuffer.size()))
                return false;
        }

        if(size == 1)
        {
            *(unsigned char*)dest = mBuffer[index - mBufferIndex];
        }
        else
        {
#ifdef _DEBUG
            auto dst = (unsigned char*)dest;
            for(size_t i = 0; i < size; i++)
                dst[i] = mBuffer.at(index - mBufferIndex + i);
#else
            memcpy(dest, mBuffer.data() + (index - mBufferIndex), size);
#endif //_DEBUG
        }

        return true;
    }

    uint64_t size() override
    {
        return mFileSize;
    }

    void setbuffersize(size_t size) override
    {
        mBufferSize = size_t(std::min(uint64_t(size), mFileSize));
    }

    void setbufferdirection(Direction direction) override
    {
        mBufferDirection = direction;
    }
};

class FileLines
{
    std::vector<uint64_t> mLines;
    std::unique_ptr<IBufferedFileReader> mReader;

public:
    bool isopen()
    {
        return mReader && mReader->isopen();
    }

    bool open(const wchar_t* szFileName)
    {
        if(isopen())
            return false;
        mReader = std::make_unique<HandleFileReader>(szFileName);
        return mReader->isopen();
    }

    bool parse()
    {
        if(!isopen())
            return false;
        auto filesize = mReader->size();
        mReader->setbufferdirection(IBufferedFileReader::Right);
        mReader->setbuffersize(10 * 1024 * 1024);
        size_t curIndex = 0, curSize = 0;
        for(uint64_t i = 0; i < filesize; i++)
        {
            /*if (mLines.size() % 100000 == 0)
                printf("%zu\n", i);*/
            char ch;
            if(!mReader->readchar(i, ch))
                return false;
            if(ch == '\r')
                continue;
            if(ch == '\n')
            {
                mLines.push_back(curIndex);
                curIndex = i + 1;
                curSize = 0;
                continue;
            }
            curSize++;
        }
        if(curSize > 0)
            mLines.push_back(curIndex);
        mLines.push_back(filesize + 1);
        return true;
    }

    size_t size() const
    {
        return mLines.size() - 1;
    }

    std::string operator[](size_t index)
    {
        auto lineStart = mLines[index];
        auto nextLineStart = mLines[index + 1];
        std::string result;
        mReader->readstring(lineStart, nextLineStart - lineStart - 1, result);
        while(!result.empty() && result.back() == '\r')
            result.pop_back();
        return result;
    }
};