#ifndef _XREFS_H
#define _XREFS_H

#include "_global.h"
#include "jansson/jansson_x64dbg.h"

bool XrefAdd(duint Address, duint From);
bool XrefGet(duint Address, XREF_INFO* List);
duint XrefGetCount(duint Address);
XREFTYPE XrefGetType(duint Address);
bool XrefDeleteAll(duint Address);
void XrefDelRange(duint Start, duint End);
void XrefCacheSave(rapidjson::Document & Root);
void XrefCacheLoad(rapidjson::Document & Root);
void XrefClear();

#endif // _FUNCTION_H