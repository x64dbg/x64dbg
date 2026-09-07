#pragma once

#include <deque>
#include "_plugins.h"

class DbCallbackBatcher
{
public:
    explicit DbCallbackBatcher(bool loading = false);
    ~DbCallbackBatcher();

    static bool IsActive(bool loading = false);
    static void Add(DbOperation & op, bool loading = false);

private:
    static thread_local DbCallbackBatcher* tActiveBatcher; // per thread batch object

    void add(DbOperation & op, bool loading);
    void flush();

    std::vector<DbOperation> mOperations;
    std::deque<std::string> mStrings;
    DbCallbackBatcher* mPrevious = nullptr;
    uint32_t mBatchId = 0;
    bool mLoading = false;
    bool mActive = false;
    bool mFlushing = false;
    bool mOwner = false; // if a batcher is defined in a thread where there's already a batcher on the stack, this is set to false
};
