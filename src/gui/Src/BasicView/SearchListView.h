#ifndef SEARCHLISTVIEW_H
#define SEARCHLISTVIEW_H

#include <QWidget>
#include <QMenu>
#include <QVBoxLayout>
#include <QLineEdit>
#include "SearchListViewTable.h"

namespace Ui
{
class SearchListView;
}

class SearchListView : public QWidget
{
    Q_OBJECT

public:
    explicit SearchListView(QWidget* parent = 0);
    ~SearchListView();

    QVBoxLayout* mMainLayout;
    SearchListViewTable* mList;
    SearchListViewTable* mSearchList;
    SearchListViewTable* mCurList;
    QLineEdit* mSearchBox;
    int mSearchStartCol;

    bool findTextInList(SearchListViewTable* list, QString text, int row, int startcol, bool startswith);

private slots:
    void searchTextChanged(const QString & arg1);
    void listContextMenu(const QPoint & pos);
    void doubleClickedSlot();
    void searchSlot();
    void on_checkBoxRegex_toggled(bool checked);

signals:
    void enterPressedSignal();
    void listContextMenuSignal(QMenu* wMenu);
    void emptySearchResult();

private:
    Ui::SearchListView* ui;
    QVBoxLayout* mListLayout;
    QWidget* mListPlaceHolder;
    QAction* mSearchAction;
    int mCursorPosition;

protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

#endif // SEARCHLISTVIEW_H
