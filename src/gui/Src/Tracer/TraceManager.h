#pragma once

#include <QTabWidget>
#include <QPushButton>
#include <QMap>
#include "TraceWidget.h"

class MRUList;

class TraceManager : public QTabWidget
{
    Q_OBJECT
public:
    explicit TraceManager(QWidget* parent = nullptr);
    ~TraceManager() override;

    void contextMenuEvent(QContextMenuEvent* event) override;

public slots:
    void open();
    void openSlot(const QString &);
    void closeTab(int index);
    void closeAllTabs();
    void toggleTraceRecording();

private:
    MRUList* mMRUList;
    QPushButton* mCloseAllTabs;
};
