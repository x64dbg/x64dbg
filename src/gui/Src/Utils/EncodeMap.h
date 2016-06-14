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
    duint getDataSize(duint va, duint codesize);
    ENCODETYPE getDataType(duint addr, duint codesize);
    void setDataType(duint va, ENCODETYPE type);
    void setDataType(duint va, duint size, ENCODETYPE type);
    bool isRangeConflict(duint offset, duint size, duint codesize);
    duint getEncodeTypeSize(ENCODETYPE type);

protected:
    duint mBase;
    duint mSize;
    byte* mBuffer;
};

#endif // ENCODEMAP_H
