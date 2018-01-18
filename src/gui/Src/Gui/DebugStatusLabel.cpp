#include "DebugStatusLabel.h"
#include <QTextDocument>

DebugStatusLabel::DebugStatusLabel(QStatusBar* parent) : QLabel(parent)
{
    this->setFrameStyle(QFrame::Sunken | QFrame::Panel); //sunken style
    this->setStyleSheet("QLabel { background-color : #C0C0C0; }");

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
    this->setFixedHeight(fm.height() + 5);
    this->setAlignment(Qt::AlignCenter);
    this->setFixedWidth(maxWidth + 10);
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));

}

void DebugStatusLabel::debugStateChangedSlot(DBGSTATE state)
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
