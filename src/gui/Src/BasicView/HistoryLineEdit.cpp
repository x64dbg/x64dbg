#include "HistoryLineEdit.h"
#include "Bridge.h"

HistoryLineEdit::HistoryLineEdit(QWidget* parent) : QLineEdit(parent)
{
    mCmdIndex = -1;
    bSixPressed = false;
}

void HistoryLineEdit::loadSettings(QString sectionPrefix)
{
    char buffer[MAX_SETTING_SIZE];
    for(int i = 1; BridgeSettingGet(sectionPrefix.toUtf8().constData(),
                                    QString("Line%1").arg(i).toUtf8().constData(),
                                    buffer) && buffer[0] && i < mCmdHistoryMaxSize; i++)
    {
        QString entry = QString(buffer);
        mCmdHistory.append(entry);
    }
}

void HistoryLineEdit::saveSettings(QString sectionPrefix)
{
    int i = 1;
    for(i = 1; i <= mCmdHistory.size(); i++)
    {
        BridgeSettingSet(sectionPrefix.toUtf8().constData(),
                         QString("Line%1").arg(i).toUtf8().constData(),
                         mCmdHistory.at(i - 1).toUtf8().constData());
    }

    // Sentinel in case we saved less than is in the store currently
    BridgeSettingSet(sectionPrefix.toUtf8().constData(),
                     QString("Line%1").arg(i).toUtf8().constData(),
                     "");
}

void HistoryLineEdit::addLineToHistory(QString parLine)
{
    if(mCmdHistory.size() > mCmdHistoryMaxSize)
        mCmdHistory.removeLast();

    if(mCmdHistory.empty() || mCmdHistory.first() != parLine)
        mCmdHistory.prepend(parLine);

    mCmdIndex = -1;
}

QString HistoryLineEdit::addHistoryClear()
{
    auto str = text();
    if(str.length())
    {
        addLineToHistory(str);
        clear();
    }
    return str;
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

        // Set the new text if an existing command was available
        QString newText("");

        if(mCmdIndex != -1)
            newText = mCmdHistory.at(mCmdIndex);

        // NOTE: "Unlike textChanged(), this signal [textEdited()] is not emitted when
        // the text is changed programmatically, for example, by calling setText()."
        setText(newText);
        emit textEdited(newText);
    }

    QLineEdit::keyPressEvent(event);
}

void HistoryLineEdit::setFocus()
{
    mCmdIndex = -1;
    QLineEdit::setFocus();
}

