#include "DebugStatusLabel.h"
#include <QAccessible>
#include <QStyle>
#include <QMetaEnum>

DebugStatusLabel::DebugStatusLabel(QStatusBar* parent) : QLabel(parent)
{
    mStatusTexts[0] = tr("Initialized");
    mStatusTexts[1] = tr("Paused");
    mStatusTexts[2] = tr("Running");
    mStatusTexts[3] = tr("Terminated");
    mTracingText = tr("Tracing");
    QFontMetrics fm(this->font());
    int maxWidth = 0;
    for(size_t i = 0; i < _countof(mStatusTexts); i++)
    {
        int width = fm.width(mStatusTexts[i]);
        if(width > maxWidth)
            maxWidth = width;
    }
    if(fm.width(mTracingText) > maxWidth)
        maxWidth = fm.width(mTracingText);
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

    QString text = mStatusTexts[state];
    if(state == running && DbgFunctions()->IsTracing && DbgFunctions()->IsTracing())
        text = mTracingText;

    this->setText(text);
    this->mState = states[state];

    if(state == stopped)
    {
        GuiUpdateWindowTitle("");
    }

    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();

    // Passive status labels are not focused, so screen readers such as
    // Narrator do not announce QLabel's automatic NameChanged notification.
    // Preserve the explicit ValueChanged event used for live status updates.
    if(QAccessible::isActive())
    {
        QAccessibleValueChangeEvent updateEvent(this, text);
        QAccessible::updateAccessibility(&updateEvent);
    }
}
