#include "ShortcutEdit.h"
#include <QKeyEvent>
#include <QDebug>


ShortcutEdit::ShortcutEdit(QWidget *parent) :
    QLineEdit(parent)
{
}

void ShortcutEdit::keyPressEvent(QKeyEvent *event)
{
    int keyInt = event->key();
    const Qt::Key key = static_cast<Qt::Key>(keyInt);


    if( key == Qt::Key_unknown )
        return;

    if( key == Qt::Key_Escape || key == Qt::Key_Backspace )
    {
        setText("");
        return;
    }


    Qt::KeyboardModifiers modifiers = event->modifiers();
    if(modifiers.testFlag(Qt::ShiftModifier))
        keyInt += Qt::SHIFT;
    if(modifiers.testFlag(Qt::ControlModifier))
        keyInt += Qt::CTRL;
    if(modifiers.testFlag(Qt::AltModifier))
        keyInt += Qt::ALT;

    setText( QKeySequence(keyInt).toString(QKeySequence::NativeText) );
    event->setAccepted(true);
}
