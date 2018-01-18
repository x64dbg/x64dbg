#ifndef HISTORYCONTEXT_H
#define HISTORYCONTEXT_H

#include "_global.h"
#include "TitanEngine/TitanEngine.h"

/**
 * @brief The class used to hold history context.
**/
class HistoryContext
{
public:
    HistoryContext();
    ~HistoryContext();

    void restore();

protected:
    TITAN_ENGINE_CONTEXT_t registers;
    struct ChangedData
    {
        duint addr;
        char oldvalue[32];
    };
    std::vector<ChangedData> ChangedLocation;
    bool invalid;
};

void HistoryAdd();
void HistoryRestore();
void HistoryClear();
bool HistoryIsEmpty();

#endif //HISTORY_CONTEXT_H
