#pragma once

#include <DbgEng.h>

// Enable kernel debugger functions
#define KDEBUGGER_ENABLE
// Enable virtual machine IPC data transfer
#define KDEBUGGER_VMIPC_EXT

#include "windbg_output.h"
#include "windbg_event.h"

#include "kdoffset.h"
#include "kdstruct.h"
#include "kdstate.h"
#include "sym.h"
#include "kdinit.h"
#include "kdevent.h"
#include "nt.h"
#include "kdcmd.h"

#include "vmclient/vmclient.h"
#include "processor/stdafx.h"
#include "mem/stdafx.h"
#include "module/stdafx.h"

#include "PatchGuard.h"

#include "kdssdt.h"
#include "kdcontext.h"
#include "opemu.h"