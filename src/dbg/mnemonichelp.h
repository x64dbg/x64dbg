#pragma once

#include "_global.h"

class MnemonicHelp
{
public:
    static String getUniversalMnemonic(const String & mnem);
    static String getDescription(const char* mnem, int depth = 0);
    static String getBriefDescription(const char* mnem);
};