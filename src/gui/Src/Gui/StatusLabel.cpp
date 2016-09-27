#include "StatusLabel.h"
#include <QTextDocument>

StatusLabel::StatusLabel(QStatusBar* parent) : QLabel(parent)
{
    this->setFrameStyle(QFrame::Sunken | QFrame::Panel); //sunken style
    this->setStyleSheet("QLabel { background-color : #C0C0C0; }");
    if(parent) //the debug-status label only has a parent
    {
        statusTexts[0] = tr("Initialized");
        statusTexts[1] = tr("Paused");
        statusTexts[2] = tr("Running");
        statusTexts[3] = tr("Terminated");
        QFontMetrics fm(this->font());
        int maxWidth = 0;
        for(size_t i = 0; i < _countof(statusTexts); i++)
        {
            int width = fm.width(statusTexts[i]);
            if(width > maxWidth)
                maxWidth = width;
        }
        this->setTextFormat(Qt::RichText); //rich text
        parent->setStyleSheet("QStatusBar { background-color: #C0C0C0; } QStatusBar::item { border: none; }");
        this->setFixedHeight(parent->height());
        this->setAlignment(Qt::AlignCenter);
        this->setFixedWidth(maxWidth + 10);
        connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    }
    else //last log message
    {
        this->setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        connect(Bridge::getBridge(), SIGNAL(addMsgToLog(QString)), this, SLOT(logUpdate(QString)));
        connect(Bridge::getBridge(), SIGNAL(addMsgToStatusBar(QString)), this, SLOT(logUpdate(QString)));
    }
}

void StatusLabel::debugStateChangedSlot(DBGSTATE state)
{
    switch(state)
    {
    case initialized:
        this->setText(QString("<font color='#00DD00'>%1</font>").arg(statusTexts[state]));
        this->setStyleSheet("QLabel { background-color : #C0C0C0; }");
        break;
    case paused:
        this->setText(QString("<font color='#FF0000'>%1</font>").arg(statusTexts[state]));
        this->setStyleSheet("QLabel { background-color : #FFFF00; }");
        break;
    case running:
        this->setText(QString("<font color='#000000'>%1</font>").arg(statusTexts[state]));
        this->setStyleSheet("QLabel { background-color : #C0C0C0; }");
        break;
    case stopped:
        this->setText(QString("<font color='#FF0000'>%1</font>").arg(statusTexts[state]));
        this->setStyleSheet("QLabel { background-color : #C0C0C0; }");
        GuiUpdateWindowTitle("");
        break;
    }

    this->update();
}

void StatusLabel::logUpdate(QString message)
{
    if(!message.length())
        return;
    labelText += message.replace("\r\n", "\n");
    QStringList lineList = labelText.split('\n');
    labelText = lineList.last(); //if the last character is a newline this will be an empty string
    QString finalLabel;
    for(int i = 0; i < lineList.length(); i++)
    {
        const QString & line = lineList[lineList.size() - i - 1];
        if(line.length()) //set the last non-empty string from the split
        {
            finalLabel = line;
            break;
        }
    }
    setText(finalLabel);
}
