#pragma once

#include <QObject>
#include "Imports.h"

class MemoryPage : public QObject
{
    Q_OBJECT
public:
    explicit MemoryPage(duint parBase, duint parSize, QObject* parent = nullptr);

    virtual bool read(void* parDest, dsint parRVA, duint parSize) const;
    virtual bool write(const void* parDest, dsint parRVA, duint parSize);
    duint getSize() const;
    duint getBase() const;
    duint va(dsint rva) const;
    void setAttributes(duint base, duint size);
    bool inRange(duint va) const;

protected:
    duint mBase;
    duint mSize;
};
