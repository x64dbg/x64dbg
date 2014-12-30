#include "StatusLabel.h"

StatusLabel::StatusLabel(QStatusBar* parent) : QLabel(parent)
{
    this->setFrameStyle(QFrame::Sunken | QFrame::Panel); //sunken style
    this->setStyleSheet("QLabel { background-color : #c0c0c0; }");
    this->setTextFormat(Qt::RichText); //rich text
    if(parent) //the debug-status label only has a parent
    {
        parent->setStyleSheet("QStatusBar { background-color: #c0c0c0; } QStatusBar::item { border: none; }");
        this->setFixedHeight(parent->height());
        this->setAlignment(Qt::AlignCenter);
        this->setFixedWidth(65);
        connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    }
    else //last log message
    {
        connect(Bridge::getBridge(), SIGNAL(addMsgToLog(QString)), this, SLOT(logUpdate(QString)));
        connect(Bridge::getBridge(), SIGNAL(addMsgToStatusBar(QString)), this, SLOT(logUpdate(QString)));
    }
}

void StatusLabel::debugStateChangedSlot(DBGSTATE state)
{
    switch(state)
    {
    case initialized:
        this->setText("<font color='#00DD00'>Initialized</font>");
        this->setStyleSheet("QLabel { background-color : #c0c0c0; }");
        break;
    case paused:
        this->setText("<font color='#ff0000'>Paused</font>");
        this->setStyleSheet("QLabel { background-color : #ffff00; }");
        break;
    case running:
        this->setText("Running");
        this->setStyleSheet("QLabel { background-color : #c0c0c0; }");
        break;
    case stopped:
        this->setText("<font color='#ff0000'>Terminated</font>");
        this->setStyleSheet("QLabel { background-color : #c0c0c0; }");
        GuiUpdateWindowTitle("");
        break;
    default:
        break;
    }
    this->repaint();
}

void StatusLabel::logUpdate(QString message)
{
    if(labelText.contains(QChar('\n'))) //newline
        labelText = "";
    labelText += message;
    setText(labelText);
    this->repaint();
}
