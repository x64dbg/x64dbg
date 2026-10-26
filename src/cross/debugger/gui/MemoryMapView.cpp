#include "gui/MemoryMapView.h"

#include <Configuration.h>
#include <MiscUtil.h>
#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QPainter>
#include <QProgressDialog>
#include <RichTextPainter.h>
#include <StringUtil.h>
#include <algorithm>

namespace
{
    constexpr int kDumpSteps = 1000;

    struct SectionContent
    {
        const char* name;
        const char* content;
        bool prefix;
    };

    const SectionContent kSectionContents[] =
    {
        {".text", QT_TRANSLATE_NOOP("MemoryMapView", "Executable code"), false},
        {".init", QT_TRANSLATE_NOOP("MemoryMapView", "Initialization code"), false},
        {".fini", QT_TRANSLATE_NOOP("MemoryMapView", "Termination code"), false},
        {".rodata", QT_TRANSLATE_NOOP("MemoryMapView", "Read-only initialized data"), false},
        {".data", QT_TRANSLATE_NOOP("MemoryMapView", "Initialized data"), false},
        {".data.rel.ro", QT_TRANSLATE_NOOP("MemoryMapView", "Relocated read-only data"), false},
        {".bss", QT_TRANSLATE_NOOP("MemoryMapView", "Uninitialized data"), false},
        {".tdata", QT_TRANSLATE_NOOP("MemoryMapView", "Thread-local storage"), false},
        {".dynamic", QT_TRANSLATE_NOOP("MemoryMapView", "Dynamic linking information"), false},
        {".dynsym", QT_TRANSLATE_NOOP("MemoryMapView", "Dynamic symbols"), false},
        {".dynstr", QT_TRANSLATE_NOOP("MemoryMapView", "Dynamic strings"), false},
        {".relr.dyn", QT_TRANSLATE_NOOP("MemoryMapView", "Relocations"), false},
        {".gcc_except_table", QT_TRANSLATE_NOOP("MemoryMapView", "Exception information"), false},
        {".init_array", QT_TRANSLATE_NOOP("MemoryMapView", "Constructors"), false},
        {".fini_array", QT_TRANSLATE_NOOP("MemoryMapView", "Destructors"), false},
        {".interp", QT_TRANSLATE_NOOP("MemoryMapView", "Program interpreter"), false},
        {".hash", QT_TRANSLATE_NOOP("MemoryMapView", "Symbol hash table"), false},
        {".gnu.hash", QT_TRANSLATE_NOOP("MemoryMapView", "Symbol hash table"), false},
        {".plt", QT_TRANSLATE_NOOP("MemoryMapView", "Procedure linkage table"), true},
        {".got", QT_TRANSLATE_NOOP("MemoryMapView", "Global offset table"), true},
        {".rela", QT_TRANSLATE_NOOP("MemoryMapView", "Relocations"), true},
        {".eh_frame", QT_TRANSLATE_NOOP("MemoryMapView", "Exception information"), true},
        {".note", QT_TRANSLATE_NOOP("MemoryMapView", "Notes"), true},
        {".gnu.version", QT_TRANSLATE_NOOP("MemoryMapView", "Symbol versions"), true},
    };

    QString sectionContent(const QString & section)
    {
        for(const auto & entry : kSectionContents)
        {
            const QLatin1String name(entry.name);
            if(entry.prefix ? section.startsWith(name) : section == name)
                return MemoryMapView::tr(entry.content);
        }
        return QString();
    }

    QString typeText(const ElfBugRegionType type)
    {
        switch(type)
        {
        case ElfBugRegionType_Image:
            return QStringLiteral("IMG");
        case ElfBugRegionType_Mapped:
            return QStringLiteral("MAP");
        default:
            return QStringLiteral("PRV");
        }
    }
}

MemoryMapView::MemoryMapView(DbgAdapter* adapter, QWidget* parent)
    : StdIconTable(parent)
    , mAdapter(adapter)
{
    const int charwidth = getCharWidth();
    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Address"), true, "", SortBy::AsHex);
    addColumnAt(8 + charwidth * 2 * sizeof(duint), tr("Size"), true, "", SortBy::AsHex);
    addColumnAt(charwidth * 9, tr("Party"), true);
    addColumnAt(8 + charwidth * 32, tr("Info"), true, tr("Page Information"));
    addColumnAt(8 + charwidth * 28, tr("Content"), true, tr("Content of section"));
    addColumnAt(8 + charwidth * 5, tr("Type"), true, tr("Allocation Type"));
    addColumnAt(8 + charwidth * 11, tr("Protection"), true, tr("Current Protection"));
    setIconColumn(ColParty);
    setAddressColumn(ColAddress);
    enableMultiSelection(true);

    setupContextMenu();

    connect(mAdapter, &DbgAdapter::stopped, this, [this](const duint rip) { mRip = rip; }, Qt::QueuedConnection);
    connect(mAdapter, &DbgAdapter::memoryMapUpdated, this, &MemoryMapView::onMemoryMapUpdated, Qt::QueuedConnection);
    connect(mAdapter, &DbgAdapter::dumpProgress, this, &MemoryMapView::onDumpProgress, Qt::QueuedConnection);
    connect(mAdapter, &DbgAdapter::dumpFinished, this, &MemoryMapView::onDumpFinished, Qt::QueuedConnection);
    connect(this, &AbstractStdTable::contextMenuSignal, this, &MemoryMapView::onContextMenu);
    connect(this, &AbstractStdTable::doubleClickedSignal, this, &MemoryMapView::onFollow);
    connect(this, &AbstractTableView::enterPressedSignal, this, &MemoryMapView::onFollow);
    connect(this, &AbstractStdTable::selectionChanged, this, &MemoryMapView::onSelectionChanged);
    connect(this, &AbstractStdTable::sortChangedSignal, this, &MemoryMapView::restoreSelection);

    mAdapter->setMemoryMapSectionView(!ConfigBool("Engine", "ListAllPages"));
}

void MemoryMapView::setupContextMenu()
{
    mGotoRipAction = new QAction(DIcon("cbp"), tr("Go to &RIP"), this);
    mGotoRipAction->setShortcut(ConfigShortcut("ActionGotoOrigin"));
    mGotoRipAction->setShortcutContext(Qt::WidgetShortcut);
    connect(mGotoRipAction, &QAction::triggered, this, &MemoryMapView::onGotoRip);
    addAction(mGotoRipAction);

    mContextMenu = new QMenu(this);
    mContextMenu->addAction(DIcon("processor64"), tr("Follow in &Disassembler"), this, &MemoryMapView::onFollowDisassembly);
    mContextMenu->addAction(DIcon("dump"), tr("&Follow in Dump"), this, &MemoryMapView::onFollowDump);
    mDumpMemoryAction = mContextMenu->addAction(DIcon("binary_save"), tr("&Dump Memory to File"), this, &MemoryMapView::onDumpMemory);
    mSwitchViewAction = mContextMenu->addAction(DIcon("change-view"), QString(), this, &MemoryMapView::onSwitchView);
    mContextMenu->addSeparator();
    mContextMenu->addAction(mGotoRipAction);
    mContextMenu->addSeparator();
    mCopyMenu = mContextMenu->addMenu(tr("&Copy"));
}

QString MemoryMapView::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    if(col == ColAddress)
    {
        const bool holdsRip = rowHolds(row, mRip);
        if(holdsRip)
            painter->fillRect(QRect(x, y, w - 1, h), QBrush(ConfigColor("MemoryMapCipBackgroundColor")));
        else if(isSelected(row))
            painter->fillRect(QRect(x, y, w, h), QBrush(mSelectionColor));
        painter->setPen(holdsRip ? ConfigColor("MemoryMapCipColor") : mTextColor);
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, getCellContent(row, col));
        return QString();
    }

    if(col == ColInfo)
    {
        const QString text = StdIconTable::paintContent(painter, row, col, x, y, w, h);
        const duint moduleBase = getCellUserdata(row, ColInfo);
        const int split = moduleBase != 0 ? text.indexOf(QStringLiteral(" \"")) : -1;
        if(split != -1)
        {
            const int sectionsEnd = text.lastIndexOf(QLatin1Char('"')) + 1;
            RichTextPainter::List richText;
            RichTextPainter::CustomRichText_t entry;
            entry.flags = RichTextPainter::FlagColor;
            const auto add = [&](const QString & part, const QColor & color)
            {
                if(part.isEmpty())
                    return;
                entry.text = part;
                entry.textColor = color;
                richText.push_back(entry);
            };
            add(text.left(split), mTextColor);
            add(text.mid(split, sectionsEnd - split), ConfigColor("MemoryMapSectionTextColor"));
            add(text.mid(sectionsEnd), mTextColor);
            RichTextPainter::paintRichText(painter, x, y, w, h, 4, richText, mFontMetrics);
            return QString();
        }
        if(moduleBase != 0 && getCellUserdata(row, ColAddress) == moduleBase)
        {
            painter->setPen(ConfigColor(getCellUserdata(row, ColParty) == ElfBugParty_User ? "SymbolUserTextColor" : "SymbolSystemTextColor"));
            painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, text);
            return QString();
        }
        return text;
    }

    if(col == ColProtection && !ConfigBool("Engine", "ListAllPages"))
    {
        const QString text = StdIconTable::paintContent(painter, row, col, x, y, w, h);
        painter->setPen(ConfigColor("MemoryMapSectionTextColor"));
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, text);
        return QString();
    }

    return StdIconTable::paintContent(painter, row, col, x, y, w, h);
}

void MemoryMapView::showEvent(QShowEvent* event)
{
    StdIconTable::showEvent(event);
    mAdapter->setMemoryMapVisible(true);
}

void MemoryMapView::hideEvent(QHideEvent* event)
{
    mAdapter->setMemoryMapVisible(false);
    StdIconTable::hideEvent(event);
}

void MemoryMapView::onMemoryMapUpdated(const QVector<DbgMemoryPage> & pages)
{
    const QString user = tr("User");
    const QString system = tr("System");
    const QIcon userIcon = DIcon("markasuser");
    const QIcon systemIcon = DIcon("markassystem");
    setRowCount(pages.size());
    for(int i = 0; i < pages.size(); ++i)
    {
        const auto & page = pages[i];
        const bool isSystem = page.party == ElfBugParty_System;
        setCellContent(i, ColAddress, ToPtrString(page.base), page.base);
        setCellContent(i, ColSize, ToPtrString(page.size), page.size);
        setCellContent(i, ColParty, isSystem ? system : user, page.party);
        setRowIcon(i, isSystem ? systemIcon : userIcon);
        setCellContent(i, ColInfo, page.info, page.moduleBase);
        setCellContent(i, ColContent, sectionContent(page.section));
        setCellContent(i, ColType, typeText(page.type));
        setCellContent(i, ColProtection, page.perms);
    }
    reloadData();
    restoreSelection();
}

void MemoryMapView::onSessionEnded()
{
    mRip = 0;
    setRowCount(0);
    setSingleSelection(0);
    setTableOffset(0);
    reloadData();
}

void MemoryMapView::onFollow()
{
    if(mAdapter->isCodePtr(getCellUserdata(getInitialSelection(), ColAddress)))
        onFollowDisassembly();
    else
        onFollowDump();
}

void MemoryMapView::onFollowDisassembly()
{
    const duint row = getInitialSelection();
    if(row < getRowCount())
        emit followDisasmRequested(getCellUserdata(row, ColAddress));
}

void MemoryMapView::onFollowDump()
{
    const duint row = getInitialSelection();
    if(row < getRowCount())
        emit followDumpRequested(getCellUserdata(row, ColAddress));
}

void MemoryMapView::onDumpMemory()
{
    QList<duint> rows = getSelection();
    std::sort(rows.begin(), rows.end(), [this](const duint a, const duint b)
    {
        return getCellUserdata(a, ColAddress) < getCellUserdata(b, ColAddress);
    });
    duint start = 0;
    duint end = 0;
    for(const duint row : rows)
    {
        if(row >= getRowCount())
            return;
        const duint base = getCellUserdata(row, ColAddress);
        if(end == 0)
            start = base;
        else if(end != base)
        {
            SimpleErrorBox(this, tr("Error"), tr("Dumping non-consecutive memory ranges is not supported!"));
            return;
        }
        end = base + getCellUserdata(row, ColSize);
    }

    char module[MAX_MODULE_SIZE] = "";
    QString prefix;
    if(mAdapter->modNameFromAddr(start, module, sizeof(module), false))
        prefix = QString::fromUtf8(module) + QLatin1Char('_');
    const QString defaultFile = QString("%1/%2%3.bin").arg(QDir::currentPath(), prefix, ToPtrString(start));
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Memory Region"), defaultFile, tr("Binary files (*.bin);;All files (*)"));
    if(path.isEmpty())
        return;

    mDumpProgress = new QProgressDialog(tr("Dumping memory..."), tr("Cancel"), 0, kDumpSteps, this);
    mDumpProgress->setWindowModality(Qt::WindowModal);
    mDumpProgress->setMinimumDuration(500);
    mDumpProgress->setAutoClose(false);
    mDumpProgress->setAutoReset(false);
    connect(mDumpProgress, &QProgressDialog::canceled, mAdapter, &DbgAdapter::cancelDump);
    mAdapter->dumpMemory(start, end - start, path);
}

void MemoryMapView::onDumpProgress(const duint done, const duint total)
{
    mDumpProgress->setValue(static_cast<int>(done * kDumpSteps / total));
}

void MemoryMapView::onDumpFinished()
{
    mDumpProgress->deleteLater();
    mDumpProgress = nullptr;
}

void MemoryMapView::onSwitchView()
{
    const bool listAllPages = !ConfigBool("Engine", "ListAllPages");
    Config()->setBool("Engine", "ListAllPages", listAllPages);
    Config()->writeBools();
    setSingleSelection(0);
    setTableOffset(0);
    mAdapter->setMemoryMapSectionView(!listAllPages);
}

void MemoryMapView::onGotoRip()
{
    if(!mAdapter->isActive())
        return;
    const duint row = findRow(mRip);
    if(row == getRowCount())
    {
        SimpleErrorBox(this, tr("Error"), tr("Address %0 not found in memory map...").arg(ToPtrString(mRip)));
        return;
    }
    scrollSelect(row);
    reloadData();
}

void MemoryMapView::onSelectionChanged()
{
    mSelectionStart = getCellUserdata(mSelection.firstSelectedIndex, ColAddress);
    mSelectionCount = mSelection.toIndex - mSelection.fromIndex;
    mSelectionSort = mSort;
}

void MemoryMapView::restoreSelection()
{
    const duint row = findRow(mSelectionStart);
    if(row == getRowCount())
        return;
    if(mSelectionSort.column != mSort.column || mSelectionSort.ascending != mSort.ascending)
    {
        scrollSelect(row);
        updateViewport();
        return;
    }
    mSelectionStart = getCellUserdata(row, ColAddress);
    mSelection.firstSelectedIndex = row;
    mSelection.fromIndex = row;
    mSelection.toIndex = std::min(row + mSelectionCount, getRowCount() - 1);
    updateViewport();
}

void MemoryMapView::onContextMenu(const QPoint & pos)
{
    if(!mAdapter->isActive() || !getRowCount())
        return;

    mDumpMemoryAction->setEnabled(!mDumpProgress);
    mSwitchViewAction->setText(ConfigBool("Engine", "ListAllPages") ? tr("Section &view") : tr("Region &view"));
    mCopyMenu->clear();
    setupCopyMenu(mCopyMenu);
    mContextMenu->exec(mapToGlobal(pos));
}

bool MemoryMapView::rowHolds(const duint row, const duint address)
{
    const duint base = getCellUserdata(row, ColAddress);
    return address >= base && address - base < getCellUserdata(row, ColSize);
}

duint MemoryMapView::findRow(const duint address)
{
    duint row = 0;
    while(row < getRowCount() && !rowHolds(row, address))
        ++row;
    return row;
}
