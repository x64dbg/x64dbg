#include "MiniHexDump.h"
#include "GotoDialog.h"
#include "Bridge.h"

MiniHexDump::MiniHexDump(Navigation* navigation, Architecture* architecture, QWidget* parent)
    : HexDump(architecture, parent)
    , mNavigation(navigation)
{
    hexAsciiSlot();
    setupMenu();
}

void MiniHexDump::loadMinidump(udmpparser::UserDumpParser* parser)
{
    mParser = parser;
    mMemPage->setAttributes(0, 0);
    setRowCount(0);
    reloadData();
}

void MiniHexDump::hexAsciiSlot()
{
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    wColDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, wColDesc);

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 16;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(8 + charwidth * 16, tr("ASCII"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void MiniHexDump::setupMenu()
{
    addMenuAction(TableAction(TR("Goto Address"), "G"), [this]()
    {
        GotoDialog gotoDialog(this);
        if(gotoDialog.exec() == QDialog::Accepted)
            emit mNavigation->gotoAddress(Navigation::Dump, gotoDialog.address());
    });
    addMenuAction(TR("Copy Address"), [this]()
    {
        Bridge::CopyToClipboard(ToPtrString(rvaToVa(getInitialSelection())));
    });
}
