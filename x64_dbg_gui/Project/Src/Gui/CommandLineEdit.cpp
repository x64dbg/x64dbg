#include "CommandLineEdit.h"
#include <QStringListModel>
#include "Bridge.h"

CommandLineEdit::CommandLineEdit(QWidget* parent) : HistoryLineEdit(parent)
{
    //Initialize QCompleter
    mCompleter = new QCompleter(QStringList(), this);
    mCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    mCompleter->setCompletionMode(QCompleter::PopupCompletion);
    this->setCompleter(mCompleter);

    //Setup signals & slots
    connect(Bridge::getBridge(), SIGNAL(autoCompleteAddCmd(QString)), this, SLOT(autoCompleteAddCmd(QString)));
    connect(Bridge::getBridge(), SIGNAL(autoCompleteDelCmd(QString)), this, SLOT(autoCompleteDelCmd(QString)));
    connect(Bridge::getBridge(), SIGNAL(autoCompleteClearAll()), this, SLOT(autoCompleteClearAll()));
}

void CommandLineEdit::autoCompleteAddCmd(const QString cmd)
{
    QStringListModel* model = (QStringListModel*)(mCompleter->model());
    QStringList stringList = model->stringList();
    stringList << cmd.split(QChar('\1'), QString::SkipEmptyParts);
    stringList.removeDuplicates();
    model->setStringList(stringList);
}

void CommandLineEdit::autoCompleteDelCmd(const QString cmd)
{
    QStringListModel* model = (QStringListModel*)(mCompleter->model());
    QStringList stringList = model->stringList();
    QStringList deleteList = cmd.split(QChar('\1'), QString::SkipEmptyParts);
    for(int i = 0; i < deleteList.size(); i++)
        stringList.removeAll(deleteList.at(i));
    model->setStringList(stringList);
}

void CommandLineEdit::autoCompleteClearAll()
{
    QStringListModel* model = (QStringListModel*)(mCompleter->model());
    model->setStringList(QStringList());
}
