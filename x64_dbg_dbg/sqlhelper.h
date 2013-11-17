#ifndef _SQLHELPER_H

#include "_global.h"

const char* sqllasterror();
bool sqlexec(sqlite3* db, const char* query);
bool sqlhasresult(sqlite3* db, const char* query);
bool sqlgettext(sqlite3* db, const char* query, char* result);
bool sqlgetuint(sqlite3* db, const char* query, uint* result);
bool sqlgetint(sqlite3* db, const char* query, int* result);
void sqlstringescape(const char* string, char* escaped_string);
bool sqlloadorsavedb(sqlite3* memory, const char* file, bool save);

#endif // _SQLHELPER_H
