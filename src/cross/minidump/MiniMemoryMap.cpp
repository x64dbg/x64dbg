#include "MiniMemoryMap.h"
#include "udmp-utils.h"
#include "GotoDialog.h"
#include "StringUtil.h"
#include "Bridge.h"

#include <QMessageBox>

MiniMemoryMap::MiniMemoryMap(Navigation* navigation, QWidget* parent)
    : StdTable(parent)
    , mNavigation(navigation)
{
    std::map<ColumnIndex, std::pair<int, QString>> columns;
    columns[ColAllocationBase] = { 16, tr("Allocation") };
    columns[ColBaseAddress] = { 16, tr("Base") };
    columns[ColRegionSize] = { 16, tr("Size") };
    columns[ColType] = { 5, tr("Type") };
    columns[ColProtect] = { 7, tr("Protect") };
    columns[ColAllocationProtect] = { 7, tr("Initial") };
    columns[ColState] = { 7, tr("State") };
    columns[ColInfo] = { 64, tr("Info") };

    int col = 0;
    for(const auto & itr : columns)
    {
        if(itr.first != col)
        {
            qDebug() << "Bad column index" << col << "!=" << itr.first;
            QApplication::exit(1);
        }
        addColumnAt(calculateColumnWidth(itr.second.first), itr.second.second, true);
        col++;
    }

    setupMenu();
}

void MiniMemoryMap::loadFileParser(FileParser* parser)
{
    mParser = parser;

    if(parser == nullptr)
    {
        // Clear the table
        setRowCount(0);
        reloadData();
        return;
    }

    auto regions = parser->MemoryRegions();
    setRowCount(regions.size());
    size_t row = 0;
    for(const MemoryRegion & region : regions)
    {
        setCellContent(row, ColBaseAddress, formatHex(region.BaseAddress), region.BaseAddress);
        setCellContent(row, ColRegionSize, formatHex(region.RegionSize), region.RegionSize);
        setCellContent(row, ColState, QString::fromStdString(region.State));
        if(region.AllocationBase != -1)
            setCellContent(row, ColAllocationBase, formatHex(region.AllocationBase), region.AllocationBase);
        else
            setCellContent(row, ColAllocationBase, {});
        setCellContent(row, ColProtect, QString::fromStdString(region.Protect));
        setCellContent(row, ColAllocationProtect, QString::fromStdString(region.AllocationProtect));
        setCellContent(row, ColType, QString::fromStdString(region.Type));
        setCellContent(row, ColInfo, QString::fromStdString(region.Info));

        row++;
    }
    scrollSelect(0);
    reloadData();
}

void MiniMemoryMap::gotoAddress(duint address)
{
    for(int row = 0; row < getRowCount(); row++)
    {
        auto base = getCellUserdata(row, ColBaseAddress);
        auto size = getCellUserdata(row, ColRegionSize);
        if(address >= base && address < base + size)
        {
            scrollSelect(row);
            return;
        }
    }
    qDebug() << "MiniMemoryMap, did not find address: " + ToPtrString(address);
}

duint MiniMemoryMap::selectedAddress()
{
    return getCellUserdata(getInitialSelection(), ColBaseAddress);
}

void MiniMemoryMap::setupMenu()
{
    addMenuAction(TableAction(TR("Goto Address"), "G"), [this]()
    {
        GotoDialog gotoDialog(this, false);
        if(gotoDialog.exec() == QDialog::Accepted)
            emit mNavigation->gotoAddress(Navigation::MemoryMap, gotoDialog.address());
    });
    auto selectedValidAddress = [this]()
    {
        return DbgMemIsValidReadPtr(selectedAddress());
    };
    addMenuAction(TableAction(TR("Follow in dump")), [this]()
    {
        mNavigation->gotoDump(selectedAddress());
    }, selectedValidAddress);
    addMenuAction(TableAction(TR("Follow in disassembler")), [this]()
    {
        mNavigation->gotoDisassembly(selectedAddress());
    }, selectedValidAddress);
    addMenuBuilder(TR("Copy"), [this](QMenu * submenu)
    {
        setupCopyMenu(submenu);
        return true;
    });
}
