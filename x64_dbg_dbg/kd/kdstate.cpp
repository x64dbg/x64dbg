#include "../_global.h"
#include "kdstate.h"

KdState_s			KdState;
KdOffsetManager		KdSymbols;
KdOffsetManager		KdFields;

#ifdef _WIN64
KDDEBUGGER_DATA64	KdDebuggerData;
#else
KDDEBUGGER_DATA32	KdDebuggerData;
#endif

void KdStateReset()
{
	// Clear any variables
	memset(&KdDebuggerData, 0, sizeof(KdDebuggerData));

	KdSymbols.Clear();
	KdFields.Clear();
}