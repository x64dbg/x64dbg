#include "Imports.h"
#include <QString>

void DbgCmdExec(const QString & cmd)
{
    DbgCmdExec(cmd.toUtf8().constData());
}

bool DbgCmdExecDirect(const QString & cmd)
{
    return DbgCmdExecDirect(cmd.toUtf8().constData());
}
