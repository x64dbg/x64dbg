#include "database_cb_batcher.h"
#include <atomic>
#include "_plugins.h"
#include "plugin_loader.h"

thread_local DbCallbackBatcher* DbCallbackBatcher::tActiveBatcher = nullptr;

DbCallbackBatcher::DbCallbackBatcher(bool loading)
    : mLoading(loading)
{
    mPrevious = tActiveBatcher;
    mOwner = (mPrevious == nullptr);

    if(mOwner)
    {
        static std::atomic_uint32_t gBatchId(1);
        mBatchId = gBatchId++;
        mActive = IsActive(loading);
        tActiveBatcher = this;
    }
}

DbCallbackBatcher::~DbCallbackBatcher()
{
    if(mOwner)
    {
        this->flush();
        DbCallbackBatcher::tActiveBatcher = mPrevious;
    }
}

bool DbCallbackBatcher::IsActive(bool loading)
{
    if(tActiveBatcher && loading == tActiveBatcher->mLoading)
        return tActiveBatcher->mActive;
    return !plugincbempty(loading ? CB_DBLOADOPERATION : CB_DBOPERATION);
}

void DbCallbackBatcher::Add(DbOperation & op, bool loading)
{
    if(tActiveBatcher != nullptr && loading == tActiveBatcher->mLoading)
    {
        tActiveBatcher->add(op, loading);
    }
    else
    {
        PLUG_CB_DBOPERATION info;
        const DbOperation* opList = &op;

        info.operations = &opList;
        info.count = 1;
        info.batchId = 0;

        plugincbcall(loading ? CB_DBLOADOPERATION : CB_DBOPERATION, &info);
    }
}

void DbCallbackBatcher::add(DbOperation & op, bool loading)
{
    ASSERT_TRUE(loading == mLoading);

    // NOTE: bad, we already paid the DbOperation construction
    if(!mActive)
        return;

    if(op.itemType == DbItemTypeComment && op.comment.text != nullptr)
    {
        mStrings.emplace_back(op.comment.text);
        op.comment.text = mStrings.back().c_str();
    }
    else if(op.itemType == DbItemTypeLabel && op.label.text != nullptr)
    {
        mStrings.emplace_back(op.label.text);
        op.label.text = mStrings.back().c_str();
    }

    mOperations.emplace_back(op);

    // Flush if we exceed the batch size threshold
    if(mOperations.size() >= 8096)
    {
        flush();
    }
}

void DbCallbackBatcher::flush()
{
    if(mFlushing)
    {
        // The outer flush drains reentrant operations after callback delivery.
        // In this rare case the pending batch may exceed the normal size limit.
        return;
    }

    mFlushing = true;
    while(!mOperations.empty())
    {
        std::vector<DbOperation> operations;
        std::deque<std::string> strings;
        operations.swap(mOperations);
        strings.swap(mStrings);

        std::vector<const DbOperation*> opList(operations.size());
        for(size_t i = 0; i < operations.size(); i++)
            opList[i] = &operations[i];

        PLUG_CB_DBOPERATION info;
        info.operations = opList.data();
        info.count = opList.size();
        info.batchId = mBatchId;

        // Reentrant operations append to the now-empty member containers and
        // are delivered by the next iteration after this callback completes.
        plugincbcall(mLoading ? CB_DBLOADOPERATION : CB_DBOPERATION, &info);
    }
    mFlushing = false;
}
