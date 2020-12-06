#include "CPUWidget.h"
#include "ui_CPUWidget.h"
#include <QDesktopWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include "CPUSideBar.h"
#include "CPUDisassembly.h"
#include "CPUMultiDump.h"
#include "CPUStack.h"
#include "CPURegistersView.h"
#include "CPUInfoBox.h"
#include "CPUArgumentWidget.h"
#include "DisassemblerGraphView.h"
#include "Configuration.h"
#include "TabWidget.h"

CPUWidget::CPUWidget(QWidget* parent) : QWidget(parent), ui(new Ui::CPUWidget)
{
    ui->setupUi(this);
    setDefaultDisposition();

    setStyleSheet("AbstractTableView:focus, CPURegistersView:focus, CPUSideBar:focus { border: 1px solid #000000; }");

    mDisas = new CPUDisassembly(this, true);
    mSideBar = new CPUSideBar(mDisas);
    mDisas->setSideBar(mSideBar);
    mArgumentWidget = new CPUArgumentWidget(this);
    mGraph = new DisassemblerGraphView(this);

    connect(mDisas, SIGNAL(tableOffsetChanged(dsint)), mSideBar, SLOT(changeTopmostAddress(dsint)));
    connect(mDisas, SIGNAL(viewableRowsChanged(int)), mSideBar, SLOT(setViewableRows(int)));
    connect(mDisas, SIGNAL(selectionChanged(dsint)), mSideBar, SLOT(setSelection(dsint)));
    connect(mGraph, SIGNAL(detachGraph()), this, SLOT(detachGraph()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), mSideBar, SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(updateSideBar()), mSideBar, SLOT(reload()));
    connect(Bridge::getBridge(), SIGNAL(updateArgumentView()), mArgumentWidget, SLOT(refreshData()));
    connect(Bridge::getBridge(), SIGNAL(focusDisasm()), this, SLOT(setDisasmFocus()));
    connect(Bridge::getBridge(), SIGNAL(focusGraph()), this, SLOT(setGraphFocus()));

    mDisas->setCodeFoldingManager(mSideBar->getCodeFoldingManager());

    ui->mTopLeftUpperHSplitter->setCollapsible(0, true); //allow collapsing of the side bar

    ui->mTopLeftUpperLeftFrameLayout->addWidget(mSideBar);
    ui->mTopLeftUpperRightFrameLayout->addWidget(mDisas);
    ui->mTopLeftUpperRightFrameLayout->addWidget(mGraph);
    mGraph->hide();
    disasMode = 0;
    mGraphWindow = nullptr;

    ui->mTopLeftVSplitter->setCollapsible(1, true); //allow collapsing of the InfoBox
    connect(ui->mTopLeftVSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMoved(int, int)));

    mInfo = new CPUInfoBox();
    ui->mTopLeftLowerFrameLayout->addWidget(mInfo);
    int height = mInfo->getHeight();
    ui->mTopLeftLowerFrame->setMinimumHeight(height + 2);

    connect(mDisas, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));

    mDump = new CPUMultiDump(mDisas, 5, this); //dump widget
    ui->mBotLeftFrameLayout->addWidget(mDump);

    mGeneralRegs = new CPURegistersView(this);
    mGeneralRegs->setFixedWidth(1000);
    mGeneralRegs->ShowFPU(true);

    QScrollArea* upperScrollArea = new QScrollArea(this);
    upperScrollArea->setFrameShape(QFrame::NoFrame);
    upperScrollArea->setWidget(mGeneralRegs);

    QPushButton* button_changeview = new QPushButton("", this);
    connect(button_changeview, SIGNAL(clicked()), mGeneralRegs, SLOT(onChangeFPUViewAction()));
    mGeneralRegs->SetChangeButton(button_changeview);

    ui->mTopRightVSplitter->setCollapsible(1, true); //allow collapsing of the ArgumentWidget
    connect(ui->mTopRightVSplitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterMoved(int, int)));

    ui->mTopRightUpperFrameLayout->addWidget(button_changeview);
    ui->mTopRightUpperFrameLayout->addWidget(upperScrollArea);

    ui->mTopRightLowerFrameLayout->addWidget(mArgumentWidget);

    mStack = new CPUStack(mDump, 0); //stack widget
    ui->mBotRightFrameLayout->addWidget(mStack);

    // load column config
    mDisas->loadColumnFromConfig("CPUDisassembly");
    mStack->loadColumnFromConfig("CPUStack");
}

inline void saveSplitter(QSplitter* splitter, QString name)
{
    BridgeSettingSet("Main Window Settings", (name + "Geometry").toUtf8().constData(), splitter->saveGeometry().toBase64().data());
    BridgeSettingSet("Main Window Settings", (name + "State").toUtf8().constData(), splitter->saveState().toBase64().data());
}

inline void loadSplitter(QSplitter* splitter, QString name)
{
    char setting[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("Main Window Settings", (name + "Geometry").toUtf8().constData(), setting))
        splitter->restoreGeometry(QByteArray::fromBase64(QByteArray(setting)));
    if(BridgeSettingGet("Main Window Settings", (name + "State").toUtf8().constData(), setting))
        splitter->restoreState(QByteArray::fromBase64(QByteArray(setting)));
    splitter->splitterMoved(1, 0);
}

void CPUWidget::saveWindowSettings()
{
    saveSplitter(ui->mVSplitter, "mVSplitter");
    saveSplitter(ui->mTopHSplitter, "mTopHSplitter");
    saveSplitter(ui->mTopLeftVSplitter, "mTopLeftVSplitter");
    saveSplitter(ui->mTopLeftUpperHSplitter, "mTopLeftUpperHSplitter");
    saveSplitter(ui->mTopRightVSplitter, "mTopRightVSplitter");
    saveSplitter(ui->mBotHSplitter, "mBotHSplitter");
}

void CPUWidget::loadWindowSettings()
{
    loadSplitter(ui->mVSplitter, "mVSplitter");
    loadSplitter(ui->mTopHSplitter, "mTopHSplitter");
    loadSplitter(ui->mTopLeftVSplitter, "mTopLeftVSplitter");
    loadSplitter(ui->mTopLeftUpperHSplitter, "mTopLeftUpperHSplitter");
    loadSplitter(ui->mTopRightVSplitter, "mTopRightVSplitter");
    loadSplitter(ui->mBotHSplitter, "mBotHSplitter");
}

CPUWidget::~CPUWidget()
{
    delete mGraphWindow;
    delete ui;
}

void CPUWidget::setDefaultDisposition()
{
    // This is magic, don't touch it...

    // Vertical Splitter
    ui->mVSplitter->setStretchFactor(0, 48);
    ui->mVSplitter->setStretchFactor(1, 62);

    // Top Horizontal Splitter
    ui->mTopHSplitter->setStretchFactor(0, 77);
    ui->mTopHSplitter->setStretchFactor(1, 23);

    // Bottom Horizontal Splitter
    ui->mBotHSplitter->setStretchFactor(0, 60);
    ui->mBotHSplitter->setStretchFactor(1, 40);

    // Top Right Vertical Splitter
    ui->mTopRightVSplitter->setStretchFactor(0, 87);
    ui->mTopRightVSplitter->setStretchFactor(1, 13);

    // Top Left Vertical Splitter
    ui->mTopLeftVSplitter->setStretchFactor(0, 99);
    ui->mTopLeftVSplitter->setStretchFactor(1, 1);

    // Top Left Upper Horizontal Splitter
    ui->mTopLeftUpperHSplitter->setStretchFactor(0, 36);
    ui->mTopLeftUpperHSplitter->setStretchFactor(1, 64);
}

void CPUWidget::setDisasmFocus()
{
    if(disasMode == 1)
    {
        mGraph->hide();
        mDisas->show();
        mSideBar->show();
        disasMode = 0;
        connect(mDisas, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));
        disconnect(mGraph, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));
    }
    else if(disasMode == 2)
    {
        activateWindow();
    }
    mDisas->setFocus();
}

void CPUWidget::setGraphFocus()
{
    if(disasMode == 0)
    {
        mDisas->hide();
        mSideBar->hide();
        mGraph->show();
        disasMode = 1;
        disconnect(mDisas, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));
        connect(mGraph, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));
    }
    else if(disasMode == 2)
    {
        mGraph->activateWindow();
    }
    mGraph->setFocus();
}

void CPUWidget::detachGraph()
{
    if(mGraphWindow == nullptr)
    {
        mGraphWindow = new MHDetachedWindow(this);

        mGraphWindow->setWindowModality(Qt::NonModal);

        // Find Widget and connect
        connect(mGraphWindow, SIGNAL(OnClose(QWidget*)), this, SLOT(attachGraph(QWidget*)));

        mGraphWindow->setWindowTitle(tr("Graph"));
        mGraphWindow->setWindowIcon(mGraph->windowIcon());
        mGraphWindow->mNativeName = "";

        mGraph->setParent(mGraphWindow);
        ui->mTopLeftUpperRightFrameLayout->removeWidget(mGraph);

        // Create and show
        mGraphWindow->show();
        mGraphWindow->setCentralWidget(mGraph);

        // Needs to be done explicitly
        mGraph->showNormal();
        QRect screenGeometry = QApplication::desktop()->screenGeometry();
        int w = 640;
        int h = 480;
        int x = (screenGeometry.width() - w) / 2;
        int y = (screenGeometry.height() - h) / 2;
        mGraphWindow->showNormal();
        mGraphWindow->setGeometry(x, y, w, h);
        mGraphWindow->showNormal();

        disasMode = 2;

        mDisas->show();
        mSideBar->show();
        connect(mDisas, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));
        connect(mGraph, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));
    }
}

void CPUWidget::attachGraph(QWidget* widget)
{
    mGraph->setParent(this);
    ui->mTopLeftUpperRightFrameLayout->addWidget(mGraph);
    mGraph->hide();
    mGraphWindow->close();
    disconnect(mGraph, SIGNAL(selectionChanged(dsint)), mInfo, SLOT(disasmSelectionChanged(dsint)));
    delete mGraphWindow;
    mGraphWindow = nullptr;
    disasMode = 0;
}

//This is used in run to selection
duint CPUWidget::getSelectionVa()
{
    if(disasMode < 2)
        return disasMode == 0 ? mDisas->getSelectedVa() : mGraph->get_cursor_pos();
    else
        return !mGraph->hasFocus() ? mDisas->getSelectedVa() : mGraph->get_cursor_pos();
}

CPUSideBar* CPUWidget::getSidebarWidget()
{
    return mSideBar;
}

CPUDisassembly* CPUWidget::getDisasmWidget()
{
    return mDisas;
}

DisassemblerGraphView* CPUWidget::getGraphWidget()
{
    return mGraph;
}

CPUMultiDump* CPUWidget::getDumpWidget()
{
    return mDump;
}

CPUInfoBox* CPUWidget::getInfoBoxWidget()
{
    return mInfo;
}

CPUStack* CPUWidget::getStackWidget()
{
    return mStack;
}

void CPUWidget::splitterMoved(int pos, int index)
{
    Q_UNUSED(pos);
    Q_UNUSED(index);
    auto splitter = qobject_cast<QSplitter*>(sender());
    if(splitter == nullptr) {} // ???
    else if(splitter->sizes().at(1) == 0)
    {
        splitter->handle(1)->setCursor(Qt::UpArrowCursor);
        splitter->setStyleSheet("QSplitter::handle:vertical { border-top: 2px solid grey; }");
    }
    else
    {
        splitter->handle(1)->setCursor(Qt::SplitVCursor);
        splitter->setStyleSheet("");
    }
}
