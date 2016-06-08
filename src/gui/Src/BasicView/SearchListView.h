#ifndef SEARCHLISTVIEW_H
#define SEARCHLISTVIEW_H

#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include "SearchListViewTable.h"

namespace Ui
{
    class SearchListView;
}

class SearchListView : public QWidget
{
    Q_OBJECT

public:
    explicit SearchListView(bool EnableRegex = true, QWidget* parent = 0);
    ~SearchListView();

    SearchListViewTable* mList;
    SearchListViewTable* mSearchList;
    SearchListViewTable* mCurList;
    QLineEdit* mSearchBox;
    int mSearchStartCol;

    bool findTextInList(SearchListViewTable* list, QString text, int row, int startcol, bool startswith);
    void refreshSearchList();

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

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    QCheckBox* mRegexCheckbox;
    QAction* mSearchAction;
};

#endif // SEARCHLISTVIEW_H
