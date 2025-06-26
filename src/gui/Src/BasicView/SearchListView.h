#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include "MenuBuilder.h"
#include "ActionHelpers.h"
#include "AbstractSearchList.h"

class SearchListView : public QWidget, public ActionHelper<SearchListView>
{
    Q_OBJECT

public:
    SearchListView(QWidget* parent, AbstractSearchList* abstractSearchList, bool enableRegex, bool enableLock);

    AbstractStdTable* mCurList = nullptr;
    duint mSearchStartCol = 0;

    void refreshSearchList();
    void clearFilter();
    bool isSearchBoxLocked();

    // TODO: allow dynamically enable/disable regex/lock

private slots:
    void filterEntries();
    void searchTextEdited(const QString & text);
    void listContextMenu(const QPoint & pos);
    void doubleClickedSlot();
    void searchSlot();
    void on_checkBoxRegex_stateChanged(int state);

signals:
    void enterPressedSignal();
    void listContextMenuSignal(QMenu* menu);
    void emptySearchResult();
    void selectionChanged(duint row);

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    QLineEdit* mSearchBox;
    QCheckBox* mRegexCheckbox;
    QCheckBox* mLockCheckbox;
    QAction* mSearchAction;
    QTimer* mTypingTimer;
    QString mFilterText;

    AbstractSearchList* mAbstractSearchList;
};
