#include "ValidateExpressionThread.h"

ValidateExpressionThread::ValidateExpressionThread(QObject* parent) : QThread(parent), mExpressionChanged(false), mStopThread(false)
{
    this->mOnExpressionChangedCallback = nullptr;
}

void ValidateExpressionThread::start()
{
    mStopThread = false;
    QThread::start();
}

void ValidateExpressionThread::stop()
{
    mStopThread = true;
}

void ValidateExpressionThread::emitExpressionChanged(bool validExpression, bool validPointer, dsint value)
{
    emit expressionChanged(validExpression, validPointer, value);
}

void ValidateExpressionThread::emitInstructionChanged(dsint sizeDifference, QString error)
{
    emit instructionChanged(sizeDifference, error);
}

void ValidateExpressionThread::textChanged(QString text)
{
    mExpressionMutex.lock();
    if(mExpressionText != text)
    {
        mExpressionChanged = true;
        mExpressionText = text;
    }
    mExpressionMutex.unlock();
}

void ValidateExpressionThread::additionalStateChanged()
{
    mExpressionMutex.lock();
    mExpressionChanged = true;
    mExpressionMutex.unlock();
}

void ValidateExpressionThread::setOnExpressionChangedCallback(EXPRESSIONCHANGEDCB callback)
{
    mOnExpressionChangedCallback = callback;
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
        if(changed && mOnExpressionChangedCallback != nullptr)
        {
            mOnExpressionChangedCallback(expression);
        }
        Sleep(50);
    }
}
