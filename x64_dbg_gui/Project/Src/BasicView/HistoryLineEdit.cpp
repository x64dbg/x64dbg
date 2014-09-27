#include "HistoryLineEdit.h"

HistoryLineEdit::HistoryLineEdit(QWidget* parent) : QLineEdit(parent)
{
    mCmdHistory.clear();
    mCmdIndex = -1;
    bSixPressed = false;
}

void HistoryLineEdit::addLineToHistory(QString parLine)
{
    mCmdHistory.prepend(parLine);

    if(mCmdHistory.size() > 32)
        mCmdHistory.removeLast();

    mCmdIndex = -1;
}

void HistoryLineEdit::keyPressEvent(QKeyEvent* event)
{
    int wKey = event->key();

    //This fixes a very annoying bug on some systems
    if(bSixPressed)
    {
        bSixPressed = false;
        if(event->text() == "^")
        {
            event->ignore();
            return;
        }
    }
    if(wKey == Qt::Key_6)
        bSixPressed = true;

    if(wKey == Qt::Key_Up || wKey == Qt::Key_Down)
    {
        if(wKey == Qt::Key_Up)
            mCmdIndex++;
        else if(wKey == Qt::Key_Down)
            mCmdIndex--;

        mCmdIndex = mCmdIndex < -1 ? -1 : mCmdIndex;
        mCmdIndex = mCmdIndex > mCmdHistory.size() - 1 ? mCmdHistory.size() - 1 : mCmdIndex;

        if(mCmdIndex == -1)
        {
            setText("");
        }
        else
        {
            setText(mCmdHistory.at(mCmdIndex));
        }
    }

    QLineEdit::keyPressEvent(event);
}

void HistoryLineEdit::setFocus()
{
    mCmdIndex = -1;
    QLineEdit::setFocus();
}

