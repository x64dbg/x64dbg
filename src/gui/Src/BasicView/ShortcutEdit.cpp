#include "ShortcutEdit.h"
#include <QStyle>

ShortcutEdit::ShortcutEdit(QWidget* parent) : QLineEdit(parent)
{
    keyInt = -1;
    mError = true;
}

const QKeySequence ShortcutEdit::getKeysequence() const
{
    if(keyInt == -1) //return empty on -1
        return QKeySequence();
    // returns current keystroke combination
    return QKeySequence(keyInt);
}

bool ShortcutEdit::error() const
{
    return mError;
}

void ShortcutEdit::setErrorState(bool error)
{
    this->mError = error;

    this->style()->unpolish(this);
    this->style()->polish(this);
}

void ShortcutEdit::keyPressEvent(QKeyEvent* event)
{
    keyInt = event->key();
    // find key-id
    const Qt::Key key = static_cast<Qt::Key>(keyInt);

    // we do not know how to handle this case
    if(key == Qt::Key_unknown)
    {
        keyInt = -1;
        emit askForSave();
        return;
    }

    // Workaround for Shift+Tab
    if(keyInt == Qt::Key_Backtab)
        keyInt = Qt::Key_Tab;

    // any combination of "Ctrl, Alt, Shift" ?
    Qt::KeyboardModifiers modifiers = event->modifiers();
    QString text = event->text();
    // The shift modifier only counts when it is not used to type a symbol
    // that is only reachable using the shift key anyway
    if(modifiers.testFlag(Qt::ShiftModifier) && (text.isEmpty() ||
            !text.at(0).isPrint() ||
            text.at(0).isLetterOrNumber() ||
            text.at(0).isSpace()))
        keyInt += Qt::SHIFT;
    if(modifiers.testFlag(Qt::ControlModifier))
        keyInt += Qt::CTRL;
    if(modifiers.testFlag(Qt::AltModifier))
        keyInt += Qt::ALT;

    // some strange cases (only Ctrl)
    QString KeyText = QKeySequence(keyInt).toString(QKeySequence::NativeText);
    for(int i = 0; i < KeyText.length(); i++)
    {
        if(KeyText[i].toLatin1() == 0)
        {
            setText("");
            keyInt = -1;
            emit askForSave();
            return;
        }
    }

    // display key combination
    setText(QKeySequence(keyInt).toString(QKeySequence::NativeText));
    // do not forward keypress-event
    event->setAccepted(true);

    // everything is fine , so ask for saving
    emit askForSave();
}
