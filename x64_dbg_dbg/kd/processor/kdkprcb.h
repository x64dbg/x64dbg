#pragma once

bool KdKprcbLoadSymbols();

ULONG64 KdKprcbForProcessor	(ULONG Processor);
bool	KdKprcbContextGet	(CONTEXT *Context, ULONG Processor);
bool	KdKprcbContextSet	(CONTEXT *Context, ULONG Processor);