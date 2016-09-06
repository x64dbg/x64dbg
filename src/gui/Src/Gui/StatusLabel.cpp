#include "StatusLabel.h"
#include <QTextDocument>

StatusLabel::StatusLabel(QStatusBar* parent) : QLabel(parent)
{
    this->setFrameStyle(QFrame::Sunken | QFrame::Panel); //sunken style
    this->setStyleSheet("QLabel { background-color : #c0c0c0; }");
    if(parent) //the debug-status label only has a parent
    {
        this->setTextFormat(Qt::RichText); //rich text
        parent->setStyleSheet("QStatusBar { background-color: #c0c0c0; } QStatusBar::item { border: none; }");
        this->setFixedHeight(parent->height());
        this->setAlignment(Qt::AlignCenter);
        this->setFixedWidth(65);
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
        this->setText(tr("<font color='#00DD00'>Initialized</font>"));
        this->setStyleSheet("QLabel { background-color : #c0c0c0; }");
        break;
    case paused:
        this->setText(tr("<font color='#ff0000'>Paused</font>"));
        this->setStyleSheet("QLabel { background-color : #ffff00; }");
        break;
    case running:
        this->setText(tr("Running"));
        this->setStyleSheet("QLabel { background-color : #c0c0c0; }");
        break;
    case stopped:
        this->setText(tr("<font color='#ff0000'>Terminated</font>"));
        this->setStyleSheet("QLabel { background-color : #c0c0c0; }");
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
