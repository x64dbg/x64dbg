#pragma once

#include "_global.h"
#include "jansson/jansson_x64dbg.h"

void* EncodeMapGetBuffer(duint addr, duint* size, bool create = false);
void EncodeMapReleaseBuffer(void* buffer);
ENCODETYPE EncodeMapGetType(duint addr, duint codesize);
duint EncodeMapGetSize(duint addr, duint codesize);
void EncodeMapDelSegment(duint addr);
void EncodeMapDelRange(duint start, duint end);
bool EncodeMapSetType(duint addr, duint size, ENCODETYPE type, bool* created = nullptr);
void EncodeMapCacheSave(JSON Root);
void EncodeMapCacheLoad(JSON Root);
void EncodeMapClear();
duint GetEncodeTypeSize(ENCODETYPE type);