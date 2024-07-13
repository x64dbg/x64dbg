#pragma once

#include <QTabWidget>
#include <QPushButton>
#include <QMap>
#include "TabWidget.h"

class MRUList;

class TraceManager : public MHTabWidget
{
    Q_OBJECT
public:
    explicit TraceManager(QWidget* parent = nullptr);
    ~TraceManager() override;

    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    void displayLogWidget();

public slots:
    void open();
    void openSlot(const QString &);
    void DeleteTab(int index) override;
    void closeAllTabs();
    void toggleTraceRecording();

private:
    MRUList* mMRUList;
};
