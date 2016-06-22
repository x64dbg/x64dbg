#ifndef ENCODEMAP_H
#define ENCODEMAP_H

#include <QObject>
#include "bridge/bridgemain.h"

class EncodeMap : public QObject
{
    Q_OBJECT
public:
    explicit EncodeMap(QObject* parent = 0);
    ~EncodeMap();

    void setMemoryRegion(duint va);
    duint getDataSize(duint va, duint codesize, duint tmpcodecount = 0, duint* tmpcodelist = nullptr);
    ENCODETYPE getDataType(duint addr, duint codesize, duint tmpcodecount = 0, duint* tmpcodelist = nullptr);
    void setDataType(duint va, ENCODETYPE type);
    void setDataType(duint va, duint size, ENCODETYPE type);
    void delRange(duint start, duint size);
    void delSegment(duint va);
    bool isRangeConflict(duint offset, duint size, duint codesize, duint tmpcodecount = 0, duint* tmpcodelist = nullptr);
    bool isDataType(ENCODETYPE type);
    duint getEncodeTypeSize(ENCODETYPE type);
    bool isCode(ENCODETYPE type) {return type == enc_unknown || type == enc_code; }

protected:
    duint mBase;
    duint mSize;
    byte* mBuffer;
};

#endif // ENCODEMAP_H
