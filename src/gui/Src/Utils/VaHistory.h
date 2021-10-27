#pragma once

#include "Bridge.h"

class VaHistory
{
public:
    void addVaToHistory(duint parVa)
    {
        //truncate everything right from the current VA
        if(mVaHistory.size() && mCurrentVa < mVaHistory.size() - 1) //mCurrentVa is not the last
            mVaHistory.erase(mVaHistory.begin() + mCurrentVa + 1, mVaHistory.end());

        //do not have 2x the same va in a row
        if(!mVaHistory.size() || mVaHistory.back() != parVa)
        {
            mCurrentVa++;
            mVaHistory.push_back(parVa);
        }
    }

    bool historyHasPrev()
    {
        return !(!mCurrentVa || !mVaHistory.size()); //we are at the earliest history entry
    }

    bool historyHasNext()
    {
        return !(!mVaHistory.size() || mCurrentVa >= mVaHistory.size() - 1); //we are at the newest history entry
    }

    duint historyPrev()
    {
        if(!historyHasPrev())
            return 0;
        mCurrentVa--;
        return mVaHistory.at(mCurrentVa);
    }

    duint historyNext()
    {
        if(!historyHasNext())
            return 0;
        mCurrentVa++;
        return mVaHistory.at(mCurrentVa);
    }

    void historyClear()
    {
        mCurrentVa = -1;
        mVaHistory.clear();
    }

private:
    std::vector<duint> mVaHistory;
    size_t mCurrentVa = -1;
};
