#pragma once

#include "_global.h"

struct LABELSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_LABEL_SIZE];
    bool manual;
};

bool LabelSet(uint Address, const char* Text, bool Manual);
bool LabelFromString(const char* Text, uint* Address);
bool LabelGet(uint Address, char* Text);
bool LabelDelete(uint Address);
void LabelDelRange(uint Start, uint End);
void LabelCacheSave(JSON root);
void LabelCacheLoad(JSON root);
bool LabelEnum(LABELSINFO* List, size_t* Size);
void LabelClear();