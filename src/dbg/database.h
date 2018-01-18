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
void DbClear(bool terminating = false);
void DbSetPath(const char* Directory, const char* ModulePath);
bool DbCheckHash(duint currentHash);
duint DbGetHash();

#endif // _DATABASE_H