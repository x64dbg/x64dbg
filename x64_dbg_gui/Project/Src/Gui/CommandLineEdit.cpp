#include "CommandLineEdit.h"

CommandLineEdit::CommandLineEdit(QWidget *parent) : QLineEdit(parent)
{
    mCmdHistory.clear();
    mCmdIndex = -1;
}


void CommandLineEdit::addCmdToHistory(QString parCmd)
{
    mCmdHistory.prepend(parCmd);

    if(mCmdHistory.size() > 32)
        mCmdHistory.removeLast();

    mCmdIndex = -1;
}


void CommandLineEdit::keyPressEvent(QKeyEvent* event)
{
    int wKey = event->key();

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



void CommandLineEdit::setFocusToCmd()
{
    mCmdIndex = -1;
    setFocus();
}

