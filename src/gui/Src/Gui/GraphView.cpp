
#include "GraphView.h"

void deleteControlFlowGraph(ControlFlowGraph* ctrlFlowGraph)
{
    ctrlFlowGraph->deleteLater();
}


GraphView::GraphView(QWidget *parent) :
    QWidget(parent),
    bProgramInitialized(false),
    mVLayout(new QVBoxLayout()),
    mControlFlowGraph(new ControlFlowGraph, deleteControlFlowGraph)
{
    mVLayout->addWidget(mControlFlowGraph.get());
    setLayout(mVLayout);

    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChangedSlot(DBGSTATE)));
}

GraphView::~GraphView()
{
    mVLayout->deleteLater();
}

void GraphView::drawGraphAtSlot(duint va)
{

    // TODO : Fix this, start analysis from debugger rather than from GUI
    if(!bProgramInitialized)
    {
        mControlFlowGraph->startControlFlowAnalysis();
        bProgramInitialized = true;
    }

    mControlFlowGraph->drawGraphAtSlot(va);
}

void GraphView::dbgStateChangedSlot(DBGSTATE state)
{
    if(state == initialized)
    {
        if(mControlFlowGraph.get() == nullptr)
            mControlFlowGraph = std::make_unique<ControlFlowGraph>();

        mVLayout->addWidget(mControlFlowGraph.get());
    }
    else if(state == stopped)
    {
        mVLayout->removeWidget(mControlFlowGraph.get());
        mControlFlowGraph.reset();
        bProgramInitialized = false;
    }
}

