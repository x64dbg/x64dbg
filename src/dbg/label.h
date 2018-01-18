#ifndef _LABEL_H
#define _LABEL_H

#include "_global.h"
#include "addrinfo.h"

struct LABELSINFO : AddrInfo
{
    std::string text;
};

bool LabelSet(duint Address, const char* Text, bool Manual, bool Temp = false);
bool LabelFromString(const char* Text, duint* Address);
bool LabelGet(duint Address, char* Text);
bool LabelDelete(duint Address);
void LabelDelRange(duint Start, duint End, bool Manual);
void LabelCacheSave(JSON root);
void LabelCacheLoad(JSON root);
void LabelClear();
void LabelGetList(std::vector<LABELSINFO> & list);
bool LabelGetInfo(duint Address, LABELSINFO* info);

#endif // _LABEL_H