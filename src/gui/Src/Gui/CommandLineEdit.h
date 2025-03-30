#pragma once

#include "bridge/bridgemain.h"
#include "HistoryLineEdit.h"
#include <QAbstractItemView>
#include <QCompleter>
#include <QComboBox>
#include <QStringListModel>

class CommandLineEdit : public HistoryLineEdit
{
    Q_OBJECT

public:
    explicit CommandLineEdit(QWidget* parent = nullptr);
    ~CommandLineEdit();

    void keyPressEvent(QKeyEvent* event);
    bool focusNextPrevChild(bool next);

    void execute();
    QWidget* selectorWidget();

public slots:
    void autoCompleteUpdate(const QString text);
    void autoCompleteAddCmd(const QString cmd);
    void autoCompleteDelCmd(const QString cmd);
    void autoCompleteClearAll();
    void registerScriptType(SCRIPTTYPEINFO* info);
    void unregisterScriptType(int id);
    void scriptTypeChanged(int index);
    void scriptTypeActivated(int index);
    void fontsUpdated();

private:
    QComboBox* mCmdScriptType;
    QCompleter* mCompleter;
    QStringListModel* mCompleterModel;
    QList<SCRIPTTYPEINFO> mScriptInfo;
    QStringList mDefaultCompletions;
    bool mDefaultCompletionsUpdated = false;
    int mCurrentScriptIndex;
};
