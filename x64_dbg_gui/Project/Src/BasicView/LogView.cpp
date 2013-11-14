#include "LogView.h"

LogView::LogView(QWidget *parent) : QTextEdit(parent)
{
    QFont wFont("Monospace", 8);
    wFont.setStyleHint(QFont::Monospace);
    wFont.setFixedPitch(true);

    this->setFont(wFont);

    this->setStyleSheet("QTextEdit { background-color: rgb(255, 251, 240) }");
    this->setUndoRedoEnabled(false);
    this->setReadOnly(true);

    connect(Bridge::getBridge(), SIGNAL(addMsgToLog(QString)), this, SLOT(addMsgToLogSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearLogSlot()));
}


void LogView::addMsgToLogSlot(QString msg)
{
    this->moveCursor(QTextCursor::End);
    this->insertPlainText(msg);
}


void LogView::clearLogSlot()
{
    this->clear();
}
