#pragma once

#include <QObject>
#include "Imports.h"

class EncodeMap : public QObject
{
    Q_OBJECT
public:
    explicit EncodeMap(QObject* parent = nullptr);
    ~EncodeMap();

    void setMemoryRegion(duint va);
    duint getDataSize(duint va, duint codesize);
    ENCODETYPE getDataType(duint addr);
    void setDataType(duint va, ENCODETYPE type);
    void setDataType(duint va, duint size, ENCODETYPE type);
    void delRange(duint start, duint size);
    void delSegment(duint va);

    static duint getEncodeTypeSize(ENCODETYPE type)
    {
        switch(type)
        {
        case enc_byte:
            return 1;
        case enc_word:
            return 2;
        case enc_dword:
            return 4;
        case enc_fword:
            return 6;
        case enc_qword:
            return 8;
        case enc_tbyte:
            return 10;
        case enc_oword:
            return 16;
        case enc_mmword:
            return 8;
        case enc_xmmword:
            return 16;
        case enc_ymmword:
            return 32;
        case enc_zmmword:
            return 64;
        case enc_real4:
            return 4;
        case enc_real8:
            return 8;
        case enc_real10:
            return 10;
        case enc_ascii:
            return 1;
        case enc_unicode:
            return 2;
        default:
            return 1;
        }
    }

    static bool isCode(ENCODETYPE type)
    {
        switch(type)
        {
        case enc_unknown:
        case enc_code:
        case enc_junk:
        case enc_middle:
            return true;
        default:
            return false;
        }
    }

    bool inBufferRange(duint addr) const
    {
        return addr >= mBase && addr < mBase + mBufferSize;
    }

protected:
    duint mBase;
    duint mSize;
    byte* mBuffer;
    duint mBufferSize;
};
