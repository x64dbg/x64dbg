#pragma once

#include <BasicView/HexDump.h>
#include "MiniDump.h"
#include "MagicMenu.h"
#include "Navigation.h"

class MiniHexDump : public HexDump, public MagicMenu<MiniHexDump>
{
    Q_OBJECT

public:
    explicit MiniHexDump(Navigation* navigation, Architecture* architecture, QWidget* parent = nullptr);
    void loadMinidump(udmpparser::UserDumpParser* parser);

private slots:
    void hexAsciiSlot();

private:
    udmpparser::UserDumpParser* mParser = nullptr;
    int mAsciiSeparator = 0;
    Navigation* mNavigation = nullptr;

    void setupMenu();
};
