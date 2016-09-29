#include "mnemonichelp.h"
#include "threading.h"

static std::unordered_map<String, String> MnemonicMap;
static std::unordered_map<String, String> MnemonicBriefMap;

bool MnemonicHelp::loadFromText(const char* json)
{
    EXCLUSIVE_ACQUIRE(LockMnemonicHelp);
    MnemonicMap.clear();
    auto root = json_loads(json, 0, 0);
    if(root)
    {
        // Get a handle to the root object -> x86-64 subtree
        auto jsonData = json_object_get(root, "x86-64");

        // Check if there was something to load
        if(jsonData)
        {
            size_t i;
            JSON value;
            json_array_foreach(jsonData, i, value)
            {
                auto mnem = json_string_value(json_object_get(value, "mnem"));
                auto description = json_string_value(json_object_get(value, "description"));
                if(mnem && description)
                    MnemonicMap[StringUtils::ToLower(mnem)] = description;
            }
        }

        // Get a handle to the root object -> x86-64-brief subtree
        auto jsonDataBrief = json_object_get(root, "x86-64-brief");

        // Check if there was something to load
        if(jsonDataBrief)
        {
            size_t i;
            JSON value;
            json_array_foreach(jsonDataBrief, i, value)
            {
                auto mnem = json_string_value(json_object_get(value, "mnem"));
                auto description = json_string_value(json_object_get(value, "description"));
                if(mnem && description)
                    MnemonicBriefMap[mnem] = description;
            }
        }

        // Free root
        json_decref(root);
    }
    else
        return false;
    return true;
}

String MnemonicHelp::getUniversalMnemonic(const String & mnem)
{
    auto mnemLower = StringUtils::ToLower(mnem);
    auto startsWith = [&](const char* n)
    {
        return StringUtils::StartsWith(mnemLower, n);
    };
    if(mnemLower == "jmp")
        return mnemLower;
    if(mnemLower == "loop")  //LOOP
        return mnemLower;
    if(startsWith("int"))  //INT n
        return "int n";
    if(startsWith("cmov"))  //CMOVcc
        return "cmovcc";
    if(startsWith("fcmov"))  //FCMOVcc
        return "fcmovcc";
    if(startsWith("j"))  //Jcc
        return "jcc";
    if(startsWith("loop"))  //LOOPcc
        return "loopcc";
    if(startsWith("set"))  //SETcc
        return "setcc";
    return mnemLower;
}

String MnemonicHelp::getDescription(const char* mnem, int depth)
{
    if(mnem == nullptr)
        return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid mnemonic!"));
    if(depth == 10)
        return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Too many redirections..."));
    SHARED_ACQUIRE(LockMnemonicHelp);
    auto mnemuni = getUniversalMnemonic(mnem);
    auto found = MnemonicMap.find(mnemuni);
    if(found == MnemonicMap.end())
    {
        if(mnemuni[0] == 'v')  //v/vm
        {
            found = MnemonicMap.find(mnemuni.c_str() + 1);
            if(found == MnemonicMap.end())
                return "";
        }
        else
            return "";
    }
    const auto & description = found->second;
    if(StringUtils::StartsWith(description, "-R:"))  //redirect
        return getDescription(description.c_str() + 3, depth + 1);
    return description;
}

String MnemonicHelp::getBriefDescription(const char* mnem)
{
    if(mnem == nullptr)
        return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Invalid mnemonic!"));
    SHARED_ACQUIRE(LockMnemonicHelp);
    auto mnemLower = StringUtils::ToLower(mnem);
    if(mnemLower == "???")
        return GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "invalid instruction"));
    auto found = MnemonicBriefMap.find(mnemLower);
    if(found == MnemonicBriefMap.end())
    {
        if(mnemLower[0] == 'v')  //v/vm
        {
            found = MnemonicBriefMap.find(mnemLower.c_str() + 1);
            if(found != MnemonicBriefMap.end())
            {
                if(mnemLower.length() > 1 && mnemLower[1] == 'm')  //vm
                    return "vm " + found->second;
                return "vector " + found->second;
            }
        }
        return "";
    }
    return found->second;
}
