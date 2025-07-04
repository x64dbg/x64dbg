#pragma once

#include <BasicView/HexDump.h>
#include "File.h"
#include "MagicMenu.h"
#include "Navigation.h"

class MiniHexDump : public HexDump, public MagicMenu<MiniHexDump>
{
    Q_OBJECT

public:
    explicit MiniHexDump(Navigation* navigation, Architecture* architecture, QWidget* parent = nullptr);
    void loadFile(File* file);

private slots:
    void hexAsciiSlot();

private:
    File* mFile = nullptr;
    int mAsciiSeparator = 0;
    Navigation* mNavigation = nullptr;

    void setupMenu();
};
