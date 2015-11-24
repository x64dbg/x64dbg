#ifndef MEMORYPAGE_H
#define MEMORYPAGE_H

#include <QObject>
#include "Imports.h"

class MemoryPage : public QObject
{
    Q_OBJECT
public:
    explicit MemoryPage(duint parBase, duint parSize, QObject* parent = 0);

    bool read(void* parDest, duint parRVA, duint parSize) const;
    bool read(byte_t* parDest, duint parRVA, duint parSize) const;
    bool write(const void* parDest, duint parRVA, duint parSize);
    bool write(const byte_t* parDest, duint parRVA, duint parSize);
    duint getSize() const;
    duint getBase() const;
    duint va(dsint rva) const;
    void setAttributes(duint base, duint size);

private:
    duint mBase;
    duint mSize;
};

#endif // MEMORYPAGE_H
