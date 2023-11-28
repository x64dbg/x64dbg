#pragma once

#include <QProgressBar>
#include <QLabel>
#include "StdSearchListView.h"

class QTabWidget;

class ReferenceView : public StdSearchListView
{
    Q_OBJECT

public:
    ReferenceView(bool sourceView = false, QWidget* parent = nullptr);
    void setupContextMenu();
    void connectBridge();
    void disconnectBridge();
    int progress() const;
    int currentTaskProgress() const;

public slots:
    void addColumnAtRef(int width, QString title);

    void setRowCount(duint count) override;

    void setSingleSelection(int index, bool scroll);
    void addCommand(QString title, QString command);
    void referenceContextMenu(QMenu* menu);
    void followAddress();
    void followDumpAddress();
    void followApiAddress();
    void followGenericAddress();
    void toggleBreakpoint();
    void setBreakpointOnAllCommands();
    void removeBreakpointOnAllCommands();
    void setBreakpointOnAllApiCalls();
    void removeBreakpointOnAllApiCalls();
    void toggleBookmark();
    void refreshShortcutsSlot();
    void referenceSetProgressSlot(int progress);
    void referenceSetCurrentTaskProgressSlot(int progress, QString taskTitle);
    void searchSelectionChanged(duint index);
    void reloadDataSlot();

signals:
    void showCpu();

private slots:
    void referenceExecCommand();

private:
    QProgressBar* mSearchTotalProgress;
    QProgressBar* mSearchCurrentTaskProgress;
    QAction* mFollowAddress;
    QAction* mFollowDumpAddress;
    QAction* mFollowApiAddress;
    QAction* mToggleBreakpoint;
    QAction* mToggleBookmark;
    QAction* mSetBreakpointOnAllCommands;
    QAction* mRemoveBreakpointOnAllCommands;
    QAction* mSetBreakpointOnAllApiCalls;
    QAction* mRemoveBreakpointOnAllApiCalls;
    bool mUpdateCountLabel = false;
    QLabel* mCountTotalLabel;
    QVector<QString> mCommandTitles;
    QVector<QString> mCommands;
    QTabWidget* mParent;

    enum BPSetAction
    {
        Enable,
        Disable,
        Toggle,
        Remove
    };

    void setBreakpointAt(duint row, BPSetAction action);
    dsint apiAddressFromString(const QString & s);

    void mouseReleaseEvent(QMouseEvent* event);
};
