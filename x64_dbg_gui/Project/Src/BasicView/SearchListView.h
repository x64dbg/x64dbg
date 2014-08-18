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
    void listKeyPressed(QKeyEvent* event);
    void listContextMenu(const QPoint & pos);
    void doubleClickedSlot();

signals:
    void enterPressedSignal();
    void listContextMenuSignal(QMenu* wMenu);

private:
    Ui::SearchListView* ui;
    QVBoxLayout* mListLayout;
    QWidget* mListPlaceHolder;

};

#endif // SEARCHLISTVIEW_H
