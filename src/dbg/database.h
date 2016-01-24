#ifndef _DATABASE_H
#define _DATABASE_H

#include "_global.h"

enum class DbLoadSaveType
{
    CommandLine,
    DebugData,
    All
};

void DbSave(DbLoadSaveType saveType);
void DbLoad(DbLoadSaveType loadType);
void DbClose();
void DbSetPath(const char* Directory, const char* ModulePath);

#endif // _DATABASE_H