#pragma once

#include "FileParser.h"
#include <BasicView/Disassembly.h>
#include "MagicMenu.h"
#include "Navigation.h"

class MiniDisassembly : public Disassembly, public MagicMenu<MiniDisassembly>
{
    Q_OBJECT

public:
    MiniDisassembly(Navigation* navigation, Architecture* architecture, QWidget* parent = nullptr);
    void loadFileParser(const FileParser* parser);

private:
    const FileParser* mParser = nullptr;
    Navigation* mNavigation = nullptr;

    void setupMenu();
};
