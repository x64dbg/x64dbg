#ifndef MEMORYPAGE_H
#define MEMORYPAGE_H

#include <QObject>
#include "NewTypes.h"

class MemoryPage : public QObject
{
    Q_OBJECT
public:
    explicit MemoryPage(uint_t parBase, uint_t parSize, QObject* parent = 0);

    bool read(void* parDest, uint_t parRVA, uint_t parSize) const;
    bool read(byte_t* parDest, uint_t parRVA, uint_t parSize) const;
    bool write(const void* parDest, uint_t parRVA, uint_t parSize) const;
    bool write(const byte_t* parDest, uint_t parRVA, uint_t parSize) const;
    uint_t getSize() const;
    uint_t getBase() const;
    uint_t va(int_t rva) const;
    void setAttributes(uint_t base, uint_t size);

private:
    uint_t mBase;
    uint_t mSize;
};

#endif // MEMORYPAGE_H
