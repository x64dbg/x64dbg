#ifndef COMMANDLINEEDIT_H
#define COMMANDLINEEDIT_H

#include "HistoryLineEdit.h"
#include <QCompleter>
#include <QComboBox>
#include <QStringListModel>

typedef bool (* SCRIPTEXECUTE)(const char* Text);
typedef void (* SCRIPTCOMPLETER)(const char* Text, char** Entries, int* EntryCount);

struct GUI_SCRIPT_INFO
{
    const char* DisplayName;
    int AssignedId;
    SCRIPTEXECUTE Execute;
    SCRIPTCOMPLETER CompleteCommand;
};

class CommandLineEdit : public HistoryLineEdit
{
    Q_OBJECT

public:
    explicit CommandLineEdit(QWidget* parent = 0);
    void keyPressEvent(QKeyEvent* event);
    bool focusNextPrevChild(bool next);

    void registerScriptType(GUI_SCRIPT_INFO* info);
    void unregisterScriptType(int Id);
    void execute();

    QWidget* selectorWidget();

public slots:
    void autoCompleteUpdate(const QString text);
    void autoCompleteAddCmd(const QString cmd);
    void autoCompleteDelCmd(const QString cmd);
    void autoCompleteClearAll();
    void scriptTypeChanged(int index);

private:
    QComboBox* mCmdScriptType;
    QCompleter* mCompleter;
    QStringListModel* mCompleterModel;
    QList<GUI_SCRIPT_INFO> mScriptInfo;
    QStringList mDefaultCompletions;
    int mCurrentScriptIndex;
};

#endif // COMMANDLINEEDIT_H
