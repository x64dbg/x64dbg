#include "MiniDisassembly.h"
#include "GotoDialog.h"
#include "Bridge.h"
#include "StringUtil.h"
#include "Memory/MemoryPage.h"

MiniDisassembly::MiniDisassembly(Navigation* navigation, Architecture* architecture, QWidget* parent)
    : Disassembly(architecture, true, parent)
    , mNavigation(navigation)
{
    setupMenu();
}

void MiniDisassembly::loadMinidump(const MiniDump::AbstractParser* parser)
{
    mParser = parser;
    mMemPage->setAttributes(0, 0);
    setRowCount(0);
    reloadData();
}

void MiniDisassembly::setupMenu()
{
    addMenuAction(TableAction(TR("Goto Address"), "G"), [this]()
    {
        GotoDialog gotoDialog(this);
        if(gotoDialog.exec() == QDialog::Accepted)
            emit mNavigation->gotoAddress(Navigation::Disassembly, gotoDialog.address());
    });
    addMenuAction(TR("Copy Address"), [this]()
    {
        Bridge::CopyToClipboard(ToPtrString(getSelectedVa()));
    });
}
