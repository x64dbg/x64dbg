#pragma once

#include "_global.h"

enum LOAD_SAVE_DB_TYPE
{
    COMMAND_LINE_ONLY,
    ALL_BUT_COMMAND_LINE,
    ALL
};

void DBSave(LOAD_SAVE_DB_TYPE saveType);
void DBLoad(LOAD_SAVE_DB_TYPE loadType);
void DBClose();
void DBSetPath(const char* Directory, const char* ModulePath);