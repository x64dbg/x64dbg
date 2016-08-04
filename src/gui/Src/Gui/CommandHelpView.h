#ifndef COMMANDHELPVIEW_H
#define COMMANDHELPVIEW_H

#include <QWidget>

class QVBoxLayout;
class SearchListView;
class StdTable;

namespace Ui
{
    class CommandHelpView;
}

class CommandHelpView : public QWidget
{
    Q_OBJECT

public:
    explicit CommandHelpView(QWidget* parent = 0);
    ~CommandHelpView();

private slots:
    void moduleSelectionChanged(int index);
    void symbolSelectionChanged(int index);

signals:
    void showCpu();

private:
    Ui::CommandHelpView* ui;
    QVBoxLayout* mMainLayout;
    SearchListView* mSearchListView;
    StdTable* mModuleList;
    int mCurrentMode;
};

#endif // COMMANDHELPVIEW_H
