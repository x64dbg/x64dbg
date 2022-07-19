#pragma once

#ifndef _BRIDGEMAIN_H_
#include "bridge/bridgemain.h"
#endif
#ifndef _DBGFUNCTIONS_H
#include "dbg/_dbgfunctions.h"
#endif
#ifndef _DBG_TYPES_H_
#include "dbg_types.h"
#endif

// Convenience overloads
class QString;

bool DbgCmdExec(const QString & cmd);
bool DbgCmdExecDirect(const QString & cmd);
