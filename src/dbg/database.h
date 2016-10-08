#ifndef _DATABASE_H
#define _DATABASE_H

#include "_global.h"

enum class DbLoadSaveType
{
    CommandLine,
    DebugData,
    All
};

void DbSave(DbLoadSaveType saveType, const char* dbfile = nullptr, bool disablecompression = false);
void DbLoad(DbLoadSaveType loadType, const char* dbfile = nullptr);
void DbClose();
void DbClear();
void DbSetPath(const char* Directory, const char* ModulePath);

#endif // _DATABASE_H