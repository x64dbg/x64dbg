#include "Imports.h"
#include <QString>

bool DbgCmdExecAsync(const QString & cmd)
{
    return DbgCmdExecAsync(cmd.toUtf8().constData());
}

bool DbgCmdExecAsyncDirect(const QString & cmd)
{
    return DbgCmdExecAsyncDirect(cmd.toUtf8().constData());
}
