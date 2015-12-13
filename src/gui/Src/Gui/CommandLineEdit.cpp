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

void CommandLineEdit::keyPressEvent(QKeyEvent *event)
{
    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Tab)
        {
            QStringListModel *strListModel = (QStringListModel*)(mCompleter->model());
            QStringList stringList = strListModel->stringList();

            if(stringList.size())
            {
                QModelIndex currentModelIndex = mCompleter->popup()->currentIndex();

                // If not item selected, select first one in the list
                if(currentModelIndex.row() < 0)
                    currentModelIndex = mCompleter->currentIndex();


                // If popup list is not visible, selected next suggested command
                if(!mCompleter->popup()->isVisible())
                {
                    for(int row=0; row < mCompleter->popup()->model()->rowCount(); row++)
                    {
                        QModelIndex modelIndex = mCompleter->popup()->model()->index(row, 0);

                        // If the lineedit contains a suggested command, get the next suggested one
                        if(mCompleter->popup()->model()->data(modelIndex) == this->text())
                        {
                            int nextModelIndexRow = (currentModelIndex.row() + 1) % mCompleter->popup()->model()->rowCount();
                            currentModelIndex = mCompleter->popup()->model()->index(nextModelIndexRow, 0);
                            break;
                        }
                    }
                }

                mCompleter->popup()->setCurrentIndex(currentModelIndex);
                mCompleter->popup()->hide();

                int currentRow = mCompleter->currentRow();
                mCompleter->setCurrentRow(currentRow);
            }
        }
        else
            HistoryLineEdit::keyPressEvent(event);
    }
    else
        HistoryLineEdit::keyPressEvent(event);
}

// Disables moving to Prev/Next child when pressing tab
bool CommandLineEdit::focusNextPrevChild(bool next)
{
    Q_UNUSED(next);
    return false;
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
