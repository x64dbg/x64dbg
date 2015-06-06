#include "ValidateExpressionThread.h"

ValidateExpressionThread::ValidateExpressionThread()
{
    mExpressionChanged = false;
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
    while(true)
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
}
