#ifndef GRAPHVIEW_H
#define GRAPHVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include "ControlFlowGraph.h"

class GraphView : public QWidget
{
    Q_OBJECT

public:
    explicit GraphView(QWidget *parent = 0);
    ~GraphView();

public slots:
    void dbgStateChangedSlot(DBGSTATE state);
    void drawGraphAtSlot(duint va);

private:
    QVBoxLayout *mVLayout;
    bool bProgramInitialized;
    std::unique_ptr<ControlFlowGraph, std::function<void(ControlFlowGraph *ctrlFlowGraph)>> mControlFlowGraph;
};

#endif // GRAPHVIEW_H
