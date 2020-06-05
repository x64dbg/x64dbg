#include "DebugStatusLabel.h"
#include <QTextDocument>
#include <QStyle>
#include <QMetaEnum>

DebugStatusLabel::DebugStatusLabel(QStatusBar* parent) : QLabel(parent)
{
    mStatusTexts[0] = tr("Initialized");
    mStatusTexts[1] = tr("Paused");
    mStatusTexts[2] = tr("Running");
    mStatusTexts[3] = tr("Terminated");
    QFontMetrics fm(this->font());
    int maxWidth = 0;
    for(size_t i = 0; i < _countof(mStatusTexts); i++)
    {
        int width = fm.width(mStatusTexts[i]);
        if(width > maxWidth)
            maxWidth = width;
    }
    this->setTextFormat(Qt::RichText); //rich text
    this->setFixedHeight(fm.height() + 5);
    this->setAlignment(Qt::AlignCenter);
    this->setFixedWidth(maxWidth + 10);
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));

}

QString DebugStatusLabel::state() const
{
    return this->mState;
}

void DebugStatusLabel::debugStateChangedSlot(DBGSTATE state)
{
    const char* states[4] = { "initialized", "paused", "running", "stopped" };

    this->setText(mStatusTexts[state]);
    this->mState = states[state];

    if(state == stopped)
    {
        GuiUpdateWindowTitle("");
    }

    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();
}
