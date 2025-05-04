#pragma once

#include <QObject>
#include <QFile>
#include "Bridge.h"

// TODO: implement as mapped file
class File : public QObject, public MemoryProvider
{
    Q_OBJECT
public:
    explicit File(duint virtualBase, const QString & fileName);
    ~File();

protected:
    bool read(duint addr, void* dest, duint size) override;
    bool getRange(duint addr, duint & base, duint & size) override;
    bool isCodePtr(duint addr) override;
    bool isValidPtr(duint addr) override;

private:
    duint mVirtualBase = 0;
    duint mSize = 0;
    QFile* mFile = nullptr;
};
