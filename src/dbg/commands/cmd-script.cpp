#include "cmd-script.h"
#include "stringformat.h"
#include "console.h"
#include "variable.h"
#include "simplescript.h"

bool cbScriptLoad(int argc, char* argv[])
{
    if(argc < 2)
        return false;
    scriptload(argv[1]);
    return true;
}

bool cbScriptMsg(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "not enough arguments!")));
        return false;
    }
    GuiScriptMessage(stringformatinline(argv[1]).c_str());
    return true;
}

bool cbScriptMsgyn(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "not enough arguments!")));
        return false;
    }
    varset("$RESULT", GuiScriptMsgyn(stringformatinline(argv[1]).c_str()), false);
    return true;
}

bool cbInstrLog(int argc, char* argv[])
{
    if(argc == 1)   //just log newline
    {
        dputs_untranslated("");
        return true;
    }
    if(argc == 2)  //inline logging: log "format {rax}"
    {
        dputs_untranslated(stringformatinline(argv[1]).c_str());
    }
    else //log "format {0} string", arg1, arg2, argN
    {
        FormatValueVector formatArgs;
        for(auto i = 2; i < argc; i++)
            formatArgs.push_back(argv[i]);
        dputs_untranslated(stringformat(argv[1], formatArgs).c_str());
    }
    return true;
}