
#include "XBytesLineEdit.h"
#include <QClipboard>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>

XBytesLineEdit::XBytesLineEdit(QWidget *parent) : QLineEdit(parent)
{
    connect(this,SIGNAL(textChanged(QString)),this,SLOT(autoMask(QString)));
    connect(this,SIGNAL(selectionChanged()),this,SLOT(modSelection()));
    mParts = 1;
    setInputMask("HH");
    setContextMenuPolicy(Qt::NoContextMenu);
}

void XBytesLineEdit::autoMask(QString content)
{
    int len = content.replace(' ',"").length();
    int remainder = len%2;
    int parts = (len-remainder)/2;

    if(parts != mParts){
        QString m("HH ");
        int backupPosition = cursorPosition();
        setInputMask(m.repeated(parts+1));
        mParts = parts;
        setCursorPosition(backupPosition);
    }
}

void XBytesLineEdit::paste(){

    QString rawClipboardText = QApplication::clipboard()->text().replace(' ',"").toUpper();

    int currentCursorPosition = cursorPosition();
    QString textBeforeCursor = QLineEdit::text().mid( 0, currentCursorPosition);
    QString textAfterCursor = QLineEdit::text().mid( currentCursorPosition, text().length()-currentCursorPosition);

    autoMask(QString("%1%2%3").arg(textBeforeCursor).arg(rawClipboardText).arg(textAfterCursor));
    setText(QString("%1%2%3").arg(textBeforeCursor).arg(rawClipboardText).arg(textAfterCursor));

}

void XBytesLineEdit::copy(){
    // copy whole content
    QApplication::clipboard()->setText(text());
    //QApplication::clipboard()->setText(selectedText().replace(' ',"").toUpper());
}

void XBytesLineEdit::cut(){
    // prevent cutting
}

QString XBytesLineEdit::text()
{
    return QLineEdit::text().replace(' ',"").toUpper();
}



void XBytesLineEdit::keyPressEvent(QKeyEvent *event){
    if(event->matches(QKeySequence::Paste)){
        paste();
    }else if(event->matches(QKeySequence::Copy)){
        copy();
    }else if(event->matches(QKeySequence::Cut)){
        //prevent cutting
    }
    else{
        return QLineEdit::keyPressEvent(event);
    }
}


void XBytesLineEdit::modSelection()
{
    // prevent Selection
    deselect();
}
