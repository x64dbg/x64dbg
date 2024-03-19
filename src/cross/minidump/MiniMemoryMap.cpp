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

void MiniMemoryMap::loadMinidump(udmpparser::UserDumpParser* parser)
{
    mParser = parser;

    if(parser == nullptr)
    {
        // Clear the table
        setRowCount(0);
        reloadData();
        return;
    }

    auto & mem = parser->GetMem();
    setRowCount(mem.size());
    size_t row = 0;
    for(const auto & itr : mem)
    {
        const udmpparser::MemBlock_t & block = itr.second;
        setCellContent(row, ColBaseAddress, formatHex(block.BaseAddress), block.BaseAddress);
        setCellContent(row, ColRegionSize, formatHex(block.RegionSize), block.RegionSize);
        setCellContent(row, ColState, StateToStringShort(block.State).c_str(), block.State);
        if(block.State != MEM_FREE)
        {
            setCellContent(row, ColAllocationBase, formatHex(block.AllocationBase), block.AllocationBase);
            setCellContent(row, ColAllocationProtect, ProtectToStringShort(block.AllocationProtect).c_str(), block.AllocationProtect);
            setCellContent(row, ColProtect, ProtectToStringShort(block.Protect).c_str(), block.Protect);
            setCellContent(row, ColType, TypeToStringShort(block.Type).c_str(), block.Type);
        }
        else
        {
            setCellContent(row, ColAllocationBase, {});
            setCellContent(row, ColAllocationProtect, {});
            setCellContent(row, ColProtect, {});
            setCellContent(row, ColType, {});
        }

        auto module = parser->GetModule(block.BaseAddress);
        if(module != nullptr)
        {
            setCellContent(row, ColInfo, QString::fromStdString(module->ModuleName), module->BaseOfImage);
        }

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
