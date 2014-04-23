#pragma once

bool KdFindSymbol		(const char *Symbol, PULONG64 Offset);
bool KdFindField		(const char *Symbol, const char *Field, PULONG Offset);
bool KdFindFieldBitType	(const char *Type, PULONG BitStart, PULONG BitSize);
bool KdGetFieldSize		(const char *Symbol, PULONG Size);
bool KdGetRegIndex		(const char *Register, PULONG Index);