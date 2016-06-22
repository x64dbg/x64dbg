#include "SymbolView.h"
#include "ui_SymbolView.h"
#include <QMessageBox>
#include "Configuration.h"
#include "Bridge.h"
#include "YaraRuleSelectionDialog.h"
#include "EntropyDialog.h"
#include "LineEditDialog.h"

SymbolView::SymbolView(QWidget* parent) : QWidget(parent), ui(new Ui::SymbolView)
{
    ui->setupUi(this);

    // Set main layout
    mMainLayout = new QVBoxLayout;
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->addWidget(ui->mainSplitter);
    setLayout(mMainLayout);

    // Create reference view
    mSearchListView = new SearchListView();
    mSearchListView->mSearchStartCol = 1;

    // Create module list
    mModuleList = new SearchListView();
    mModuleList->mSearchStartCol = 1;
    int charwidth = mModuleList->mList->getCharWidth();
    mModuleList->mList->addColumnAt(charwidth * 2 * sizeof(dsint) + 8, tr("Base"), false);
    mModuleList->mList->addColumnAt(300, tr("Module"), true);
    mModuleList->mList->addColumnAt(charwidth * 8, tr("Party"), false);
    mModuleList->mSearchList->addColumnAt(charwidth * 2 * sizeof(dsint) + 8, tr("Base"), false);
    mModuleList->mSearchList->addColumnAt(300, "Module", true);
    mModuleList->mSearchList->addColumnAt(charwidth * 8, tr("Party"), false);

    // Setup symbol list
    mSearchListView->mList->addColumnAt(charwidth * 2 * sizeof(dsint) + 8, tr("Address"), true);
    mSearchListView->mList->addColumnAt(charwidth * 6 + 8, tr("Type"), true);
    mSearchListView->mList->addColumnAt(charwidth * 80, tr("Symbol"), true);
    mSearchListView->mList->addColumnAt(2000, tr("Symbol (undecorated)"), true);

    // Setup search list
    mSearchListView->mSearchList->addColumnAt(charwidth * 2 * sizeof(dsint) + 8, tr("Address"), true);
    mSearchListView->mSearchList->addColumnAt(charwidth * 6 + 8, tr("Type"), true);
    mSearchListView->mSearchList->addColumnAt(charwidth * 80, tr("Symbol"), true);
    mSearchListView->mSearchList->addColumnAt(2000, tr("Symbol (undecorated)"), true);

    // Setup list splitter
    ui->listSplitter->addWidget(mModuleList);
    ui->listSplitter->addWidget(mSearchListView);
#ifdef _WIN64
    // mModuleList : mSymbolList = 40 : 100
    ui->listSplitter->setStretchFactor(0, 40);
    ui->listSplitter->setStretchFactor(1, 100);
#else
    // mModuleList : mSymbolList = 30 : 100
    ui->listSplitter->setStretchFactor(0, 30);
    ui->listSplitter->setStretchFactor(1, 100);
#endif //_WIN64

    // Setup log edit
    ui->symbolLogEdit->setFont(mModuleList->mList->font());
    ui->symbolLogEdit->setStyleSheet("QTextEdit { background-color: rgb(255, 251, 240) }");
    ui->symbolLogEdit->setUndoRedoEnabled(false);
    ui->symbolLogEdit->setReadOnly(true);
    // Log : List = 2 : 9
    ui->mainSplitter->setStretchFactor(1, 9);
    ui->mainSplitter->setStretchFactor(0, 2);

    //setup context menu
    setupContextMenu();

    //Signals and slots
    connect(Bridge::getBridge(), SIGNAL(repaintTableView()), this, SLOT(updateStyle()));
    connect(Bridge::getBridge(), SIGNAL(addMsgToSymbolLog(QString)), this, SLOT(addMsgToSymbolLogSlot(QString)));
    connect(Bridge::getBridge(), SIGNAL(clearLog()), this, SLOT(clearSymbolLogSlot()));
    connect(Bridge::getBridge(), SIGNAL(clearSymbolLog()), this, SLOT(clearSymbolLogSlot()));
    connect(mModuleList->mList, SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));
    connect(mModuleList->mSearchList, SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));
    connect(mModuleList, SIGNAL(emptySearchResult()), this, SLOT(emptySearchResultSlot()));
    connect(mModuleList, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(moduleContextMenu(QMenu*)));
    connect(mModuleList, SIGNAL(enterPressedSignal()), this, SLOT(moduleFollow()));
    connect(Bridge::getBridge(), SIGNAL(updateSymbolList(int, SYMBOLMODULEINFO*)), this, SLOT(updateSymbolList(int, SYMBOLMODULEINFO*)));
    connect(Bridge::getBridge(), SIGNAL(setSymbolProgress(int)), ui->symbolProgress, SLOT(setValue(int)));
    connect(Bridge::getBridge(), SIGNAL(symbolRefreshCurrent()), this, SLOT(symbolRefreshCurrent()));
    connect(mSearchListView, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(symbolContextMenu(QMenu*)));
    connect(mSearchListView, SIGNAL(enterPressedSignal()), this, SLOT(symbolFollow()));
}

SymbolView::~SymbolView()
{
    delete ui;
}

void SymbolView::setupContextMenu()
{
    //Symbols
    mFollowSymbolAction = new QAction(tr("&Follow in Disassembler"), this);
    mFollowSymbolAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mFollowSymbolAction->setShortcut(QKeySequence("enter"));
    connect(mFollowSymbolAction, SIGNAL(triggered()), this, SLOT(symbolFollow()));

    mFollowSymbolDumpAction = new QAction(tr("Follow in &Dump"), this);
    connect(mFollowSymbolDumpAction, SIGNAL(triggered()), this, SLOT(symbolFollowDump()));

    mToggleBreakpoint = new QAction(tr("Toggle Breakpoint"), this);
    mToggleBreakpoint->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mToggleBreakpoint);
    mSearchListView->mList->addAction(mToggleBreakpoint);
    mSearchListView->mSearchList->addAction(mToggleBreakpoint);
    connect(mToggleBreakpoint, SIGNAL(triggered()), this, SLOT(toggleBreakpoint()));

    mToggleBookmark = new QAction(tr("Toggle Bookmark"), this);
    mToggleBookmark->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mToggleBookmark);
    mSearchListView->mList->addAction(mToggleBookmark);
    mSearchListView->mSearchList->addAction(mToggleBookmark);
    connect(mToggleBookmark, SIGNAL(triggered()), this, SLOT(toggleBookmark()));

    //Modules
    mFollowModuleAction = new QAction(tr("&Follow in Disassembler"), this);
    mFollowModuleAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mFollowModuleAction->setShortcut(QKeySequence("enter"));
    connect(mFollowModuleAction, SIGNAL(triggered()), this, SLOT(moduleFollow()));

    mFollowModuleEntryAction = new QAction(tr("Follow &Entry Point in Disassembler"), this);
    connect(mFollowModuleEntryAction, SIGNAL(triggered()), this, SLOT(moduleEntryFollow()));

    mDownloadSymbolsAction = new QAction(tr("&Download Symbols for This Module"), this);
    connect(mDownloadSymbolsAction, SIGNAL(triggered()), this, SLOT(moduleDownloadSymbols()));

    mDownloadAllSymbolsAction = new QAction(tr("Download Symbols for &All Modules"), this);
    connect(mDownloadAllSymbolsAction, SIGNAL(triggered()), this, SLOT(moduleDownloadAllSymbols()));

    mCopyPathAction = new QAction(tr("Copy File &Path"), this);
    connect(mCopyPathAction, SIGNAL(triggered()), this, SLOT(moduleCopyPath()));

    mYaraAction = new QAction(QIcon(":/icons/images/yara.png"), tr("&Yara Memory..."), this);
    connect(mYaraAction, SIGNAL(triggered()), this, SLOT(moduleYara()));

    mYaraFileAction = new QAction(QIcon(":/icons/images/yara.png"), tr("&Yara File..."), this);
    connect(mYaraFileAction, SIGNAL(triggered()), this, SLOT(moduleYaraFile()));

    mEntropyAction = new QAction(QIcon(":/icons/images/entropy.png"), tr("Entropy..."), this);
    connect(mEntropyAction, SIGNAL(triggered()), this, SLOT(moduleEntropy()));

    mModSetUserAction = new QAction(tr("Mark as &user module"), this);
    connect(mModSetUserAction, SIGNAL(triggered()), this, SLOT(moduleSetUser()));

    mModSetSystemAction = new QAction(tr("Mark as &system module"), this);
    connect(mModSetSystemAction, SIGNAL(triggered()), this, SLOT(moduleSetSystem()));

    mModSetPartyAction = new QAction(tr("Mark as &party..."), this);
    connect(mModSetPartyAction, SIGNAL(triggered()), this, SLOT(moduleSetParty()));

    //Shortcuts
    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void SymbolView::refreshShortcutsSlot()
{
    mToggleBreakpoint->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mToggleBookmark->setShortcut(ConfigShortcut("ActionToggleBookmark"));
}

void SymbolView::updateStyle()
{
    ui->symbolLogEdit->setStyleSheet(QString("QTextEdit { color: %1; background-color: %2 }").arg(ConfigColor("AbstractTableViewTextColor").name(), ConfigColor("AbstractTableViewBackgroundColor").name()));
}

void SymbolView::addMsgToSymbolLogSlot(QString msg)
{
    ui->symbolLogEdit->moveCursor(QTextCursor::End);
    ui->symbolLogEdit->insertPlainText(msg);
}

void SymbolView::clearSymbolLogSlot()
{
    ui->symbolLogEdit->clear();
}

void SymbolView::cbSymbolEnum(SYMBOLINFO* symbol, void* user)
{
    StdTable* symbolList = (StdTable*)user;
    dsint index = symbolList->getRowCount();
    symbolList->setRowCount(index + 1);
    symbolList->setCellContent(index, 0, QString("%1").arg(symbol->addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper());
    if(symbol->decoratedSymbol)
    {
        symbolList->setCellContent(index, 2, symbol->decoratedSymbol);
    }
    if(symbol->undecoratedSymbol)
    {
        symbolList->setCellContent(index, 3, symbol->undecoratedSymbol);
    }

    if(symbol->isImported)
    {
        symbolList->setCellContent(index, 1, tr("Import"));
    }
    else
    {
        symbolList->setCellContent(index, 1, tr("Export"));
    }
}

void SymbolView::moduleSelectionChanged(int index)
{
    QString mod = mModuleList->mCurList->getCellContent(index, 1);
    if(!mModuleBaseList.count(mod))
        return;
    mSearchListView->mList->setRowCount(0);
    DbgSymbolEnumFromCache(mModuleBaseList[mod], cbSymbolEnum, mSearchListView->mList);
    mSearchListView->mList->reloadData();
    mSearchListView->mList->setSingleSelection(0);
    mSearchListView->mList->setTableOffset(0);
    mSearchListView->mSearchBox->setText("");
}

void SymbolView::updateSymbolList(int module_count, SYMBOLMODULEINFO* modules)
{
    mModuleList->mList->setRowCount(module_count);
    if(!module_count)
    {
        mSearchListView->mList->setRowCount(0);
        mSearchListView->mList->setSingleSelection(0);
        mModuleList->mList->setSingleSelection(0);
    }

    mModuleBaseList.clear();
    for(int i = 0; i < module_count; i++)
    {
        QString modName(modules[i].name);
        mModuleBaseList.insert(modName, modules[i].base);
        int party = DbgFunctions()->ModGetParty(modules[i].base);
        mModuleList->mList->setCellContent(i, 0, ToPtrString(modules[i].base));
        mModuleList->mList->setCellContent(i, 1, modName);
        switch(party)
        {
        case 0:
            mModuleList->mList->setCellContent(i, 2, tr("User"));
            break;
        case 1:
            mModuleList->mList->setCellContent(i, 2, tr("System"));
            break;
        default:
            mModuleList->mList->setCellContent(i, 2, tr("Party: %1").arg(party));
            break;
        }
    }
    mModuleList->mList->reloadData();
    //NOTE: DO NOT CALL mModuleList->refreshSearchList() IT WILL DEGRADE PERFORMANCE!
    if(modules)
        BridgeFree(modules);
}

void SymbolView::symbolContextMenu(QMenu* wMenu)
{
    if(!mSearchListView->mCurList->getRowCount())
        return;
    wMenu->addAction(mFollowSymbolAction);
    wMenu->addAction(mFollowSymbolDumpAction);
    wMenu->addSeparator();
    wMenu->addAction(mToggleBreakpoint);
    wMenu->addAction(mToggleBookmark);
}

void SymbolView::symbolRefreshCurrent()
{
    mModuleList->mList->setSingleSelection(mModuleList->mList->getInitialSelection());
}

void SymbolView::symbolFollow()
{
    DbgCmdExecDirect(QString("disasm " + mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

void SymbolView::symbolFollowDump()
{
    DbgCmdExecDirect(QString("dump " + mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    emit showCpu();
}

void SymbolView::moduleContextMenu(QMenu* wMenu)
{
    if(!DbgIsDebugging() || !mModuleList->mCurList->getRowCount())
        return;

    wMenu->addAction(mFollowModuleAction);
    wMenu->addAction(mFollowModuleEntryAction);
    wMenu->addAction(mDownloadSymbolsAction);
    wMenu->addAction(mDownloadAllSymbolsAction);
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    char szModPath[MAX_PATH] = "";
    if(DbgFunctions()->ModPathFromAddr(modbase, szModPath, _countof(szModPath)))
        wMenu->addAction(mCopyPathAction);
    wMenu->addAction(mYaraAction);
    wMenu->addAction(mYaraFileAction);
    wMenu->addAction(mEntropyAction);
    wMenu->addSeparator();
    int party = DbgFunctions()->ModGetParty(modbase);
    if(party != 0)
        wMenu->addAction(mModSetUserAction);
    if(party != 1)
        wMenu->addAction(mModSetSystemAction);
    wMenu->addAction(mModSetPartyAction);
    QMenu wCopyMenu(tr("&Copy"), this);
    mModuleList->mCurList->setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
    }
}

void SymbolView::moduleFollow()
{
    DbgCmdExecDirect(QString("disasm " + mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0) + "+1000").toUtf8().constData());
    emit showCpu();
}

void SymbolView::moduleEntryFollow()
{
    DbgCmdExecDirect(QString("disasm " + mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 1) + ":entry").toUtf8().constData());
    emit showCpu();
}

void SymbolView::moduleCopyPath()
{
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    char szModPath[MAX_PATH] = "";
    if(DbgFunctions()->ModPathFromAddr(modbase, szModPath, _countof(szModPath)))
        Bridge::CopyToClipboard(szModPath);
}

void SymbolView::moduleYara()
{
    QString modname = mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 1);
    YaraRuleSelectionDialog yaraDialog(this, QString("Yara (%1)").arg(modname));
    if(yaraDialog.exec() == QDialog::Accepted)
    {
        DbgCmdExec(QString("yaramod \"%0\",\"%1\"").arg(yaraDialog.getSelectedFile()).arg(modname).toUtf8().constData());
        emit showReferences();
    }
}

void SymbolView::moduleYaraFile()
{
    QString modname = mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 1);
    YaraRuleSelectionDialog yaraDialog(this, QString("Yara (%1)").arg(modname));
    if(yaraDialog.exec() == QDialog::Accepted)
    {
        DbgCmdExec(QString("yaramod \"%0\",\"%1\",1").arg(yaraDialog.getSelectedFile()).arg(modname).toUtf8().constData());
        emit showReferences();
    }
}

void SymbolView::moduleDownloadSymbols()
{
    DbgCmdExec(QString("symdownload \"%0\"").arg(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 1)).toUtf8().constData());
}

void SymbolView::moduleDownloadAllSymbols()
{
    DbgCmdExec("symdownload");
}

void SymbolView::toggleBreakpoint()
{
    if(!DbgIsDebugging())
        return;

    if(!mSearchListView->mCurList->getRowCount())
        return;
    QString addrText = mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 0);
    duint wVA;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &wVA))
        return;

    if(!DbgMemIsValidReadPtr(wVA))
        return;

    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
    {
        wCmd = "bc " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    else
    {
        wCmd = "bp " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }

    DbgCmdExec(wCmd.toUtf8().constData());
}

void SymbolView::toggleBookmark()
{
    if(!DbgIsDebugging())
        return;

    if(!mSearchListView->mCurList->getRowCount())
        return;
    QString addrText = mSearchListView->mCurList->getCellContent(mSearchListView->mCurList->getInitialSelection(), 0);
    duint wVA;
    if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &wVA))
        return;
    if(!DbgMemIsValidReadPtr(wVA))
        return;

    bool result;
    if(DbgGetBookmarkAt(wVA))
        result = DbgSetBookmarkAt(wVA, false);
    else
        result = DbgSetBookmarkAt(wVA, true);
    if(!result)
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("DbgSetBookmarkAt failed!"));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }
    GuiUpdateAllViews();
}

void SymbolView::moduleEntropy()
{
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    char szModPath[MAX_PATH] = "";
    if(DbgFunctions()->ModPathFromAddr(modbase, szModPath, _countof(szModPath)))
    {
        EntropyDialog entropyDialog(this);
        entropyDialog.setWindowTitle(tr("Entropy (%1)").arg(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 1)));
        entropyDialog.show();
        entropyDialog.GraphFile(QString(szModPath));
        entropyDialog.exec();
    }
}

void SymbolView::moduleSetSystem()
{
    int i = mModuleList->mCurList->getInitialSelection();
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(i, 0).toUtf8().constData());
    DbgFunctions()->ModSetParty(modbase, 1);
    mModuleList->mCurList->setCellContent(i, 2, tr("System"));
    mModuleList->mCurList->reloadData();
}

void SymbolView::moduleSetUser()
{
    int i = mModuleList->mCurList->getInitialSelection();
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(i, 0).toUtf8().constData());
    DbgFunctions()->ModSetParty(modbase, 0);
    mModuleList->mCurList->setCellContent(i, 2, tr("User"));
    mModuleList->mCurList->reloadData();
}

void SymbolView::moduleSetParty()
{
    LineEditDialog mLineEdit(this);
    int party;
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    party = DbgFunctions()->ModGetParty(modbase);
    mLineEdit.setWindowIcon(QIcon(":/icons/images/bookmark.png"));
    mLineEdit.setWindowTitle(tr("Mark the party of the module as"));
    mLineEdit.setText(QString::number(party));
    if(mLineEdit.exec() == QDialog::Accepted)
    {
        bool ok;
        party = mLineEdit.editText.toInt(&ok);
        int i = mModuleList->mCurList->getInitialSelection();
        if(ok)
        {
            DbgFunctions()->ModSetParty(modbase, party);
            switch(party)
            {
            case 0:
                mModuleList->mCurList->setCellContent(i, 2, tr("User"));
                break;
            case 1:
                mModuleList->mCurList->setCellContent(i, 2, tr("System"));
                break;
            default:
                mModuleList->mCurList->setCellContent(i, 2, tr("Party: %1").arg(party));
                break;
            }
            mModuleList->mCurList->reloadData();
        }
        else
        {
            QMessageBox msg(QMessageBox::Critical, tr("Error"), tr("The party number can only be an integer"));
            msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
            msg.exec();
        }
    }
}

void SymbolView::emptySearchResultSlot()
{
    // No result after search
    mSearchListView->mCurList->setRowCount(0);
}
