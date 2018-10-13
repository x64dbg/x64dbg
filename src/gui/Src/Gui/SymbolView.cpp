#include "SymbolView.h"
#include "ui_SymbolView.h"
#include <QMessageBox>
#include "Configuration.h"
#include "Bridge.h"
#include "YaraRuleSelectionDialog.h"
#include "EntropyDialog.h"
#include "BrowseDialog.h"
#include "StdSearchListView.h"
#include "ZehSymbolTable.h"
#include <QVBoxLayout>
#include <QProcess>
#include <QFileDialog>

class SymbolSearchList : public AbstractSearchList
{
public:
    friend class SymbolView;

    SymbolSearchList()
    {
        mList = new ZehSymbolTable();
        mList->setAddressColumn(0);
        mList->setAddressLabel(false);
        mSearchList = new ZehSymbolTable();
        mSearchList->setAddressColumn(0);
        mSearchList->setAddressLabel(false);
    }

    void lock() override
    {
        mList->mMutex.lock();
        mSearchList->mMutex.lock();
    }

    void unlock() override
    {
        mSearchList->mMutex.unlock();
        mList->mMutex.unlock();
    }

    AbstractStdTable* list() const override
    {
        return mList;
    }

    AbstractStdTable* searchList() const override
    {
        return mSearchList;
    }

    void filter(const QString & filter, FilterType type, int startColumn) override
    {
        mSearchList->setRowCount(0);
        int newRowCount = 0;
        mSearchList->mData.clear();
        mSearchList->mData.reserve(mList->mData.size());
        mSearchList->mModules = mList->mModules;
        int rows = mList->getRowCount();
        for(int i = 0; i < rows; i++)
        {
            if(rowMatchesFilter(filter, type, i, startColumn))
            {
                newRowCount++;
                mSearchList->mData.push_back(mList->mData.at(i));
            }
        }
        mSearchList->setRowCount(newRowCount);
    }

    void addAction(QAction* action)
    {
        mList->addAction(action);
        mSearchList->addAction(action);
    }

private:
    ZehSymbolTable* mList;
    ZehSymbolTable* mSearchList;
};

SymbolView::SymbolView(QWidget* parent) : QWidget(parent), ui(new Ui::SymbolView)
{
    ui->setupUi(this);
    setAutoFillBackground(false);

    // Set main layout
    mMainLayout = new QVBoxLayout;
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->addWidget(ui->mainSplitter);
    setLayout(mMainLayout);

    // Create reference view
    mSymbolSearchList = new SymbolSearchList();
    mSymbolList = new SearchListView(this, mSymbolSearchList, true, true);
    mSymbolList->mSearchStartCol = 1;

    // Create module list
    mModuleList = new StdSearchListView(this, true, false);
    mModuleList->setSearchStartCol(0);
    mModuleList->enableMultiSelection(true);
    mModuleList->setAddressColumn(0, true);
    mModuleList->noDisassemblyPopup();
    int charwidth = mModuleList->getCharWidth();
    mModuleList->addColumnAt(charwidth * 2 * sizeof(dsint) + 8, tr("Base"), true);
    mModuleList->addColumnAt(300, tr("Module"), true);
    mModuleList->addColumnAt(charwidth * 8, tr("Party"), true);
    mModuleList->addColumnAt(charwidth * 60, tr("Path"), true);
    mModuleList->loadColumnFromConfig("Module");

    // Setup list splitter
    ui->listSplitter->addWidget(mModuleList);
    ui->listSplitter->addWidget(mSymbolList);
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
    ui->symbolLogEdit->setFont(ConfigFont("Log"));

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
    connect(Bridge::getBridge(), SIGNAL(selectionSymmodGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(mModuleList->stdList(), SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));
    connect(mModuleList->stdSearchList(), SIGNAL(selectionChangedSignal(int)), this, SLOT(moduleSelectionChanged(int)));
    connect(mModuleList, SIGNAL(emptySearchResult()), this, SLOT(emptySearchResultSlot()));
    connect(mModuleList, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(moduleContextMenu(QMenu*)));
    connect(mModuleList, SIGNAL(enterPressedSignal()), this, SLOT(moduleFollow()));
    connect(Bridge::getBridge(), SIGNAL(updateSymbolList(int, SYMBOLMODULEINFO*)), this, SLOT(updateSymbolList(int, SYMBOLMODULEINFO*)));
    connect(Bridge::getBridge(), SIGNAL(setSymbolProgress(int)), ui->symbolProgress, SLOT(setValue(int)));
    connect(Bridge::getBridge(), SIGNAL(symbolRefreshCurrent()), this, SLOT(symbolRefreshCurrent()));
    connect(Bridge::getBridge(), SIGNAL(symbolSelectModule(duint)), this, SLOT(symbolSelectModule(duint)));
    connect(mSymbolList, SIGNAL(listContextMenuSignal(QMenu*)), this, SLOT(symbolContextMenu(QMenu*)));
    connect(mSymbolList, SIGNAL(enterPressedSignal()), this, SLOT(enterPressedSlot()));
    connect(Config(), SIGNAL(colorsUpdated()), this, SLOT(updateStyle()));
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(updateStyle()));
}

SymbolView::~SymbolView()
{
    delete ui;
    delete mSymbolSearchList;
}

inline void saveSymbolsSplitter(QSplitter* splitter, QString name)
{
    BridgeSettingSet("SymbolsSettings", (name + "Geometry").toUtf8().constData(), splitter->saveGeometry().toBase64().data());
    BridgeSettingSet("SymbolsSettings", (name + "State").toUtf8().constData(), splitter->saveState().toBase64().data());
}

inline void loadSymbolsSplitter(QSplitter* splitter, QString name)
{
    char setting[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("SymbolsSettings", (name + "Geometry").toUtf8().constData(), setting))
        splitter->restoreGeometry(QByteArray::fromBase64(QByteArray(setting)));
    if(BridgeSettingGet("SymbolsSettings", (name + "State").toUtf8().constData(), setting))
        splitter->restoreState(QByteArray::fromBase64(QByteArray(setting)));
    splitter->splitterMoved(1, 0);
}

void SymbolView::saveWindowSettings()
{
    saveSymbolsSplitter(ui->listSplitter, "mVSymbolsSplitter");
    saveSymbolsSplitter(ui->mainSplitter, "mHSymbolsLogSplitter");
}

void SymbolView::loadWindowSettings()
{
    loadSymbolsSplitter(ui->listSplitter, "mVSymbolsSplitter");
    loadSymbolsSplitter(ui->mainSplitter, "mHSymbolsLogSplitter");
}

void SymbolView::invalidateSymbolSource(duint base)
{
    mSymbolSearchList->lock();
    for(auto mod : mSymbolSearchList->mList->mModules)
    {
        if(mod == base)
        {
            mSymbolSearchList->mList->mData.clear();
            mSymbolSearchList->mList->mData.shrink_to_fit();
            mSymbolSearchList->mList->setRowCount(0);
            mSymbolSearchList->mSearchList->mData.clear();
            mSymbolSearchList->mSearchList->mData.shrink_to_fit();
            mSymbolSearchList->mSearchList->setRowCount(0);
            mSymbolSearchList->mSearchList->setHighlightText(QString());
            GuiSymbolLogAdd(QString("[SymbolView] reload symbols for base %1\n").arg(ToPtrString(base)).toUtf8().constData());
            // TODO: properly reload symbol list
            break;
        }
    }
    mSymbolSearchList->unlock();
}

void SymbolView::setupContextMenu()
{
    QIcon disassembler = DIcon(ArchValue("processor32.png", "processor64.png"));
    //Symbols
    mFollowSymbolAction = new QAction(disassembler, tr("&Follow in Disassembler"), this);
    connect(mFollowSymbolAction, SIGNAL(triggered()), this, SLOT(symbolFollow()));

    mFollowSymbolDumpAction = new QAction(DIcon("dump.png"), tr("Follow in &Dump"), this);
    connect(mFollowSymbolDumpAction, SIGNAL(triggered()), this, SLOT(symbolFollowDump()));

    mFollowSymbolImportAction = new QAction(DIcon("import.png"), tr("Follow &imported address"), this);
    connect(mFollowSymbolImportAction, SIGNAL(triggered(bool)), this, SLOT(symbolFollowImport()));

    mToggleBreakpoint = new QAction(DIcon("breakpoint.png"), tr("Toggle Breakpoint"), this);
    mToggleBreakpoint->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mToggleBreakpoint);
    mSymbolSearchList->addAction(mToggleBreakpoint);
    connect(mToggleBreakpoint, SIGNAL(triggered()), this, SLOT(toggleBreakpoint()));

    mToggleBookmark = new QAction(DIcon("bookmark_toggle.png"), tr("Toggle Bookmark"), this);
    mToggleBookmark->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mToggleBookmark);
    mSymbolSearchList->addAction(mToggleBookmark);
    connect(mToggleBookmark, SIGNAL(triggered()), this, SLOT(toggleBookmark()));

    //Modules
    mFollowModuleAction = new QAction(disassembler, tr("&Follow in Disassembler"), this);
    mFollowModuleAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    mFollowModuleAction->setShortcut(QKeySequence("enter"));
    connect(mFollowModuleAction, SIGNAL(triggered()), this, SLOT(moduleFollow()));

    mFollowModuleEntryAction = new QAction(disassembler, tr("Follow &Entry Point in Disassembler"), this);
    connect(mFollowModuleEntryAction, SIGNAL(triggered()), this, SLOT(moduleEntryFollow()));

    mFollowInMemMap = new QAction(DIcon("memmap_find_address_page.png"), tr("Follow in Memory Map"), this);
    mFollowInMemMap->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mFollowInMemMap);
    mModuleList->addAction(mFollowInMemMap);
    connect(mFollowInMemMap, SIGNAL(triggered()), this, SLOT(moduleFollowMemMap()));

    mDownloadSymbolsAction = new QAction(DIcon("pdb.png"), tr("&Download Symbols for This Module"), this);
    mDownloadSymbolsAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mDownloadSymbolsAction);
    mModuleList->addAction(mDownloadSymbolsAction);
    connect(mDownloadSymbolsAction, SIGNAL(triggered()), this, SLOT(moduleDownloadSymbols()));

    mDownloadAllSymbolsAction = new QAction(DIcon("pdb.png"), tr("Download Symbols for &All Modules"), this);
    mDownloadAllSymbolsAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mDownloadAllSymbolsAction);
    mModuleList->addAction(mDownloadAllSymbolsAction);
    connect(mDownloadAllSymbolsAction, SIGNAL(triggered()), this, SLOT(moduleDownloadAllSymbols()));

    mCopyPathAction = new QAction(DIcon("copyfilepath.png"), tr("Copy File &Path"), this);
    mCopyPathAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mCopyPathAction);
    mModuleList->addAction(mCopyPathAction);
    connect(mCopyPathAction, SIGNAL(triggered()), this, SLOT(moduleCopyPath()));

    mBrowseInExplorer = new QAction(DIcon("browseinexplorer.png"), tr("Browse in Explorer"), this);
    mBrowseInExplorer->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mBrowseInExplorer);
    mModuleList->addAction(mBrowseInExplorer);
    connect(mBrowseInExplorer, SIGNAL(triggered()), this, SLOT(moduleBrowse()));

    mLoadLib = new QAction(DIcon("lib_load.png"), tr("Load library..."), this);
    mLoadLib->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mLoadLib);
    mModuleList->addAction(mLoadLib);
    connect(mLoadLib, SIGNAL(triggered()), this, SLOT(moduleLoad()));

    mFreeLib = new QAction(DIcon("lib_free.png"), tr("Free library"), this);
    mFreeLib->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mFreeLib);
    mModuleList->addAction(mFreeLib);
    connect(mFreeLib, SIGNAL(triggered()), this, SLOT(moduleFree()));

    mYaraAction = new QAction(DIcon("yara.png"), tr("&Yara Memory..."), this);
    connect(mYaraAction, SIGNAL(triggered()), this, SLOT(moduleYara()));

    mYaraFileAction = new QAction(DIcon("yara.png"), tr("&Yara File..."), this);
    connect(mYaraFileAction, SIGNAL(triggered()), this, SLOT(moduleYaraFile()));

    mEntropyAction = new QAction(DIcon("entropy.png"), tr("Entropy..."), this);
    mEntropyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mEntropyAction);
    mModuleList->addAction(mEntropyAction);
    connect(mEntropyAction, SIGNAL(triggered()), this, SLOT(moduleEntropy()));

    mModSetUserAction = new QAction(DIcon("markasuser.png"), tr("Mark as &user module"), this);
    mModSetUserAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mModSetUserAction);
    mModuleList->addAction(mModSetUserAction);
    connect(mModSetUserAction, SIGNAL(triggered()), this, SLOT(moduleSetUser()));

    mModSetSystemAction = new QAction(DIcon("markassystem.png"), tr("Mark as &system module"), this);
    mModSetSystemAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mModSetSystemAction);
    mModuleList->addAction(mModSetSystemAction);
    connect(mModSetSystemAction, SIGNAL(triggered()), this, SLOT(moduleSetSystem()));

    mModSetPartyAction = new QAction(DIcon("markasparty.png"), tr("Mark as &party..."), this);
    mModSetPartyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    this->addAction(mModSetPartyAction);
    mModuleList->addAction(mModSetPartyAction);
    connect(mModSetPartyAction, SIGNAL(triggered()), this, SLOT(moduleSetParty()));

    //Shortcuts
    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void SymbolView::refreshShortcutsSlot()
{
    mToggleBreakpoint->setShortcut(ConfigShortcut("ActionToggleBreakpoint"));
    mToggleBookmark->setShortcut(ConfigShortcut("ActionToggleBookmark"));
    mModSetUserAction->setShortcut(ConfigShortcut("ActionMarkAsUser"));
    mModSetSystemAction->setShortcut(ConfigShortcut("ActionMarkAsSystem"));
    mModSetPartyAction->setShortcut(ConfigShortcut("ActionMarkAsParty"));
    mEntropyAction->setShortcut(ConfigShortcut("ActionEntropy"));
    mBrowseInExplorer->setShortcut(ConfigShortcut("ActionBrowseInExplorer"));
    mDownloadSymbolsAction->setShortcut(ConfigShortcut("ActionDownloadSymbol"));
    mDownloadAllSymbolsAction->setShortcut(ConfigShortcut("ActionDownloadAllSymbol"));
    mCopyPathAction->setShortcut(ConfigShortcut("ActionCopy"));
    mFollowInMemMap->setShortcut(ConfigShortcut("ActionFollowMemMap"));
}

void SymbolView::updateStyle()
{
    ui->symbolLogEdit->setFont(ConfigFont("Log"));
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

void SymbolView::moduleSelectionChanged(int index)
{
    Q_UNUSED(index);
    setUpdatesEnabled(false);

    std::vector<duint> selectedModules;
    for(auto index : mModuleList->mCurList->getSelection())
    {
        QString modBase = mModuleList->mCurList->getCellContent(index, 0);
        duint wVA;
        if(DbgFunctions()->ValFromString(modBase.toUtf8().constData(), &wVA))
            selectedModules.push_back(wVA);
    }

    std::vector<SYMBOLPTR> data;
    for(auto base : selectedModules)
    {
        DbgSymbolEnum(base, [](const SYMBOLPTR * info, void* userdata)
        {
            ((std::vector<SYMBOLPTR>*)userdata)->push_back(*info);
            return true; // TODO: allow aborting (enumeration in a separate thread)
        }, &data);
    }

    mSymbolSearchList->lock();
    mSymbolSearchList->mList->mModules = std::move(selectedModules);
    mSymbolSearchList->mList->mData = std::move(data);
    mSymbolSearchList->mList->setRowCount(mSymbolSearchList->mList->mData.size());
    mSymbolSearchList->unlock();
    mSymbolSearchList->mList->setSingleSelection(0);
    mSymbolSearchList->mList->setTableOffset(0);
    mSymbolSearchList->mList->reloadData();
    if(!mSymbolList->isSearchBoxLocked())
        mSymbolList->mSearchBox->setText("");
    else
        mSymbolList->refreshSearchList();

    setUpdatesEnabled(true);
}

void SymbolView::updateSymbolList(int module_count, SYMBOLMODULEINFO* modules)
{
    mModuleList->stdList()->setRowCount(module_count);
    if(!module_count)
    {
        // TODO
        //mSymbolList->mList->setRowCount(0);
        //mSymbolList->mList->setSingleSelection(0);
        mModuleList->stdList()->setSingleSelection(0);
    }

    mModuleBaseList.clear();
    for(int i = 0; i < module_count; i++)
    {
        QString modName(modules[i].name);
        mModuleBaseList.insert(modName, modules[i].base);
        int party = DbgFunctions()->ModGetParty(modules[i].base);
        mModuleList->stdList()->setCellContent(i, 0, ToPtrString(modules[i].base));
        mModuleList->stdList()->setCellUserdata(i, 0, modules[i].base);
        mModuleList->stdList()->setCellContent(i, 1, modName);
        switch(party)
        {
        case 0:
            mModuleList->stdList()->setCellContent(i, 2, tr("User"));
            break;
        case 1:
            mModuleList->stdList()->setCellContent(i, 2, tr("System"));
            break;
        default:
            mModuleList->stdList()->setCellContent(i, 2, tr("Party: %1").arg(party));
            break;
        }
        char szModPath[MAX_PATH] = "";
        if(!DbgFunctions()->ModPathFromAddr(modules[i].base, szModPath, _countof(szModPath)))
            *szModPath = '\0';
        mModuleList->stdList()->setCellContent(i, 3, szModPath);
    }
    mModuleList->stdList()->reloadData();
    //NOTE: DO NOT CALL mModuleList->refreshSearchList() IT WILL DEGRADE PERFORMANCE!
    if(modules)
        BridgeFree(modules);
}

void SymbolView::symbolContextMenu(QMenu* wMenu)
{
    if(!mSymbolList->mCurList->getRowCount())
        return;
    wMenu->addAction(mFollowSymbolAction);
    wMenu->addAction(mFollowSymbolDumpAction);
    if(mSymbolList->mCurList->getCellContent(mSymbolList->mCurList->getInitialSelection(), 1) == tr("Import"))
        wMenu->addAction(mFollowSymbolImportAction);
    wMenu->addSeparator();
    wMenu->addAction(mToggleBreakpoint);
    wMenu->addAction(mToggleBookmark);
}

void SymbolView::symbolRefreshCurrent()
{
    mModuleList->stdList()->setSingleSelection(mModuleList->stdList()->getInitialSelection());
}

void SymbolView::symbolFollow()
{
    DbgCmdExec(QString("disasm " + mSymbolList->mCurList->getCellContent(mSymbolList->mCurList->getInitialSelection(), 0)).toUtf8().constData());
}

void SymbolView::symbolFollowDump()
{
    DbgCmdExecDirect(QString("dump " + mSymbolList->mCurList->getCellContent(mSymbolList->mCurList->getInitialSelection(), 0)).toUtf8().constData());
}

void SymbolView::symbolFollowImport()
{
    auto addrText = mSymbolList->mCurList->getCellContent(mSymbolList->mCurList->getInitialSelection(), 0);
    auto addr = DbgValFromString(QString("[%1]").arg(addrText).toUtf8().constData());
    if(!DbgMemIsValidReadPtr(addr))
        return;
    if(DbgFunctions()->MemIsCodePage(addr, false))
    {
        DbgCmdExec(QString("disasm %1").arg(ToPtrString(addr)).toUtf8().constData());
    }
    else
    {
        DbgCmdExecDirect(QString("dump %1").arg(ToPtrString(addr)).toUtf8().constData());
        emit Bridge::getBridge()->getDumpAttention();
    }
}

void SymbolView::symbolSelectModule(duint base)
{
    for(dsint i = 0; i < mModuleList->stdList()->getRowCount(); i++)
    {
        if(mModuleList->stdList()->getCellUserdata(i, 0) == base)
        {
            mModuleList->stdList()->setSingleSelection(i);
            mModuleList->stdSearchList()->hide(); //This could be described as a hack, but you could also say it's like wiping sandpaper over your new white Tesla.
            mModuleList->mSearchBox->clear();
            break;
        }
    }
}

void SymbolView::enterPressedSlot()
{
    auto addr = DbgValFromString(mSymbolList->mCurList->getCellContent(mSymbolList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    if(!DbgMemIsValidReadPtr(addr))
        return;
    if(mSymbolList->mCurList->getCellContent(mSymbolList->mCurList->getInitialSelection(), 1) == tr("Import"))
        symbolFollowImport();
    else if(DbgFunctions()->MemIsCodePage(addr, false))
        symbolFollow();
    else
    {
        symbolFollowDump();
        emit Bridge::getBridge()->getDumpAttention();
    }
}

void SymbolView::moduleContextMenu(QMenu* wMenu)
{
    if(!DbgIsDebugging() || !mModuleList->mCurList->getRowCount())
        return;

    wMenu->addAction(mFollowModuleAction);
    wMenu->addAction(mFollowModuleEntryAction);
    wMenu->addAction(mFollowInMemMap);
    wMenu->addAction(mDownloadSymbolsAction);
    wMenu->addAction(mDownloadAllSymbolsAction);
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    char szModPath[MAX_PATH] = "";
    if(DbgFunctions()->ModPathFromAddr(modbase, szModPath, _countof(szModPath)))
    {
        wMenu->addAction(mCopyPathAction);
        wMenu->addAction(mBrowseInExplorer);
    }
    wMenu->addAction(mLoadLib);
    wMenu->addAction(mFreeLib);
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
    wCopyMenu.setIcon(DIcon("copy.png"));
    mModuleList->mCurList->setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
    }
}

void SymbolView::moduleFollow()
{
    DbgCmdExec(QString("disasm " + mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0) + "+1000").toUtf8().constData());
}

void SymbolView::moduleEntryFollow()
{
    //Test case: libstdc++-6.dll
    DbgCmdExec(QString("disasm \"" + mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 1) + "\":entry").toUtf8().constData());
}

void SymbolView::moduleCopyPath()
{
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    char szModPath[MAX_PATH] = "";
    if(DbgFunctions()->ModPathFromAddr(modbase, szModPath, _countof(szModPath)))
        Bridge::CopyToClipboard(szModPath);
}

void SymbolView::moduleBrowse()
{
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    char szModPath[MAX_PATH] = "";
    if(DbgFunctions()->ModPathFromAddr(modbase, szModPath, _countof(szModPath)))
    {
        QProcess::startDetached(QString("%1/explorer.exe").arg(QProcessEnvironment::systemEnvironment().value("windir")), QStringList({QString("/select,"), QString(szModPath)}));
    }
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

void SymbolView::moduleLoad()
{
    if(!DbgIsDebugging())
        return;

    BrowseDialog browse(this, tr("Select DLL"), tr("Enter the path of a DLL to load in the debuggee."), tr("DLL Files (*.dll);;All Files (*.*)"), QString(), false);
    if(browse.exec() != QDialog::Accepted && browse.path.length())
        return;
    auto fileName = browse.path;
    DbgCmdExec(QString("loadlib \"%1\"").arg(fileName.replace("\\", "\\\\")).toUtf8().constData());
}

void SymbolView::moduleFree()
{
    if(!DbgIsDebugging())
        return;

    QString moduleName = mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 1);
    if(moduleName.length() != 0)
    {
        QMessageBox::StandardButton reply;
        QString question = tr("Are you sure you want to free the module: %1?\n\nThis could introduce unexpected behaviour to your debugging session...").arg(moduleName);
        reply = QMessageBox::question(this,
                                      tr("Free Library").toUtf8().constData(),
                                      question.toUtf8().constData(),
                                      QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
            DbgCmdExec(QString("freelib %1").arg(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0)).toUtf8().constData());
    }
}

void SymbolView::toggleBreakpoint()
{
    if(!DbgIsDebugging())
        return;

    if(!mSymbolList->mCurList->getRowCount())
        return;

    auto selection = mSymbolList->mCurList->getSelection();

    for(auto selectedIdx : selection)
    {
        QString addrText = mSymbolList->mCurList->getCellContent(selectedIdx, 0);
        duint wVA;
        if(!DbgFunctions()->ValFromString(addrText.toUtf8().constData(), &wVA))
            return;

        //Import means that the address is an IAT entry so we read the actual function address
        if(mSymbolList->mCurList->getCellContent(selectedIdx, 1) == tr("Import"))
            DbgMemRead(wVA, &wVA, sizeof(wVA));

        if(!DbgMemIsValidReadPtr(wVA))
            return;

        BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
        QString wCmd;

        if((wBpType & bp_normal) == bp_normal)
        {
            wCmd = "bc " + ToPtrString(wVA);
        }
        else
        {
            wCmd = "bp " + ToPtrString(wVA);
        }

        DbgCmdExec(wCmd.toUtf8().constData());
    }
}

void SymbolView::toggleBookmark()
{
    if(!DbgIsDebugging())
        return;

    if(!mSymbolList->mCurList->getRowCount())
        return;
    QString addrText = mSymbolList->mCurList->getCellContent(mSymbolList->mCurList->getInitialSelection(), 0);
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
        msg.setWindowIcon(DIcon("compile-error.png"));
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
    DbgFunctions()->RefreshModuleList();
}

void SymbolView::moduleSetUser()
{
    int i = mModuleList->mCurList->getInitialSelection();
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(i, 0).toUtf8().constData());
    DbgFunctions()->ModSetParty(modbase, 0);
    DbgFunctions()->RefreshModuleList();
}

void SymbolView::moduleSetParty()
{
    int party;
    duint modbase = DbgValFromString(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toUtf8().constData());
    party = DbgFunctions()->ModGetParty(modbase);
    QString mLineEditeditText;
    if(SimpleInputBox(this, tr("Mark the party of the module as"), QString::number(party), mLineEditeditText, tr("0 is user module, 1 is system module."), &DIcon("bookmark.png")))
    {
        bool ok;
        party = mLineEditeditText.toInt(&ok);
        int i = mModuleList->mCurList->getInitialSelection();
        if(ok)
        {
            DbgFunctions()->ModSetParty(modbase, party);
            /* TODO: refresh module list
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
            mModuleList->mCurList->reloadData();*/
        }
        else
            SimpleErrorBox(this, tr("Error"), tr("The party number can only be an integer"));
    }
}

void SymbolView::moduleFollowMemMap()
{
    QString base = mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0);
    DbgCmdExec(("memmapdump " + base).toUtf8().constData());
}

void SymbolView::emptySearchResultSlot()
{
    // No result after search
    mSymbolList->mCurList->setRowCount(0);
}

void SymbolView::selectionGetSlot(SELECTIONDATA* selection)
{
    selection->start = selection->end = duint(mModuleList->mCurList->getCellContent(mModuleList->mCurList->getInitialSelection(), 0).toULongLong(nullptr, 16));
    Bridge::getBridge()->setResult(1);
}
