#ifndef SYMBOLVIEW_H
#define SYMBOLVIEW_H

#include <QWidget>
#include "Bridge.h"

namespace Ui
{
class SymbolView;
}

class SymbolView : public QWidget
{
    Q_OBJECT

public:
    explicit SymbolView(QWidget* parent = 0);
    ~SymbolView();
    void setupContextMenu();

private slots:
    void updateStyle();
    void addMsgToSymbolLogSlot(QString msg);
    void clearSymbolLogSlot();
    void moduleSelectionChanged(int index);
    void updateSymbolList(int module_count, SYMBOLMODULEINFO* modules);
    void symbolFollow();
    void symbolFollowDump();
    void symbolContextMenu(QMenu* wMenu);
    void symbolRefreshCurrent();
    void moduleContextMenu(const QPoint & pos);
    void moduleFollow();
    void moduleEntryFollow();
    void moduleDownloadSymbols();
    void moduleDownloadAllSymbols();
    void moduleCopyPath();
    void toggleBreakpoint();
    void toggleBookmark();
    void refreshShortcutsSlot();

signals:
    void showCpu();

private:
    Ui::SymbolView* ui;
    QVBoxLayout* mMainLayout;
    QVBoxLayout* mSymbolLayout;
    QWidget* mSymbolPlaceHolder;
    SearchListView* mSearchListView;
    StdTable* mModuleList;
    QMap<QString, uint_t> mModuleBaseList;
    QAction* mFollowSymbolAction;
    QAction* mFollowSymbolDumpAction;
    QAction* mToggleBreakpoint;
    QAction* mToggleBookmark;
    QAction* mFollowModuleAction;
    QAction* mFollowModuleEntryAction;
    QAction* mDownloadSymbolsAction;
    QAction* mDownloadAllSymbolsAction;
    QAction* mCopyPathAction;

    static void cbSymbolEnum(SYMBOLINFO* symbol, void* user);
};

#endif // SYMBOLVIEW_H
