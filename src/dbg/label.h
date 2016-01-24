#ifndef _LABEL_H
#define _LABEL_H

#include "_global.h"

struct LABELSINFO
{
    char mod[MAX_MODULE_SIZE];
    duint addr;
    char text[MAX_LABEL_SIZE];
    bool manual;
};

bool LabelSet(duint Address, const char* Text, bool Manual);
bool LabelFromString(const char* Text, duint* Address);
bool LabelGet(duint Address, char* Text);
bool LabelDelete(duint Address);
void LabelDelRange(duint Start, duint End);
void LabelCacheSave(JSON root);
void LabelCacheLoad(JSON root);
bool LabelEnum(LABELSINFO* List, size_t* Size);
void LabelClear();
void LabelGetList(std::vector<LABELSINFO> & list);
bool LabelGetInfo(duint Address, LABELSINFO* info);
void LabelEnumCb(std::function<void(const LABELSINFO & info)> cbEnum, const char* module = nullptr);

#endif // _LABEL_H