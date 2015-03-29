#pragma once

#include "_global.h"

struct LABELSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_LABEL_SIZE];
    bool manual;
};

bool LabelSet(uint addr, const char* text, bool manual);
bool labelfromstring(const char* text, uint* addr);
bool LabelGet(uint addr, char* text);
bool LabelDelete(uint addr);
void LabelDelRange(uint start, uint end);
void labelcachesave(JSON root);
void labelcacheload(JSON root);
bool LabelEnum(LABELSINFO* labellist, size_t* cbsize);
void LabelClear();