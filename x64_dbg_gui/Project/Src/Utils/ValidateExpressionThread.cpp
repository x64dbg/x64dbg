#include "ValidateExpressionThread.h"

ValidateExpressionThread::ValidateExpressionThread()
{
    mExpressionChanged = false;
    mStopThread = false;
}

void ValidateExpressionThread::stop()
{
    mStopThread = true;
}

void ValidateExpressionThread::textChanged(QString text)
{
    mExpressionMutex.lock();
    if(mExpressionText != text)
        mExpressionChanged = true;
    mExpressionText = text;
    mExpressionMutex.unlock();
}

void ValidateExpressionThread::run()
{
    while(!mStopThread)
    {
        mExpressionMutex.lock();
        QString expression = mExpressionText;
        bool changed = mExpressionChanged;
        mExpressionChanged = false;
        mExpressionMutex.unlock();
        if(changed)
        {
            duint value;
            bool validExpression = DbgFunctions()->ValFromString(expression.toUtf8().constData(), &value);
            bool validPointer = validExpression && DbgMemIsValidReadPtr(value);
            emit expressionChanged(validExpression, validPointer, value);
        }
        Sleep(50);
    }
    mStopThread = false;
}
