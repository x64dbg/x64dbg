#ifndef SEARCHLISTVIEW_H
#define SEARCHLISTVIEW_H

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include "SearchListViewTable.h"
#include "MenuBuilder.h"
#include "ActionHelpers.h"

namespace Ui
{
    class SearchListView;
}

class SearchListView : public QWidget, public ActionHelper<SearchListView>
{
    Q_OBJECT

public:
    explicit SearchListView(bool EnableRegex = true, QWidget* parent = 0, bool EnableLock = false);
    ~SearchListView();

    SearchListViewTable* mList;
    SearchListViewTable* mSearchList;
    SearchListViewTable* mCurList;
    QLineEdit* mSearchBox;
    int mSearchStartCol;
    QString mLastFirstColValue;

    bool findTextInList(SearchListViewTable* list, QString text, int row, int startcol, bool startswith);
    void refreshSearchList();

    bool isSearchBoxLocked();

private slots:
    void searchTextChanged(const QString & arg1);
    void listContextMenu(const QPoint & pos);
    void doubleClickedSlot();
    void searchSlot();
    void on_checkBoxRegex_stateChanged(int state);

signals:
    void enterPressedSignal();
    void listContextMenuSignal(QMenu* wMenu);
    void emptySearchResult();

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    QCheckBox* mRegexCheckbox;
    QCheckBox* mLockCheckbox;
    QAction* mSearchAction;

    void LoadPrevListLayout(SearchListViewTable* mPrevList);
};

#endif // SEARCHLISTVIEW_H
