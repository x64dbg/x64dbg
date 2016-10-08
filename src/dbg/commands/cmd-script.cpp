#include "cmd-script.h"
#include "stringformat.h"
#include "console.h"
#include "variable.h"
#include "simplescript.h"

CMDRESULT cbScriptLoad(int argc, char* argv[])
{
    if(argc < 2)
        return STATUS_ERROR;
    scriptload(argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbScriptMsg(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "not enough arguments!")));
        return STATUS_ERROR;
    }
    GuiScriptMessage(stringformatinline(argv[1]).c_str());
    return STATUS_CONTINUE;
}

CMDRESULT cbScriptMsgyn(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "not enough arguments!")));
        return STATUS_ERROR;
    }
    varset("$RESULT", GuiScriptMsgyn(stringformatinline(argv[1]).c_str()), false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLog(int argc, char* argv[])
{
    if(argc == 1)   //just log newline
    {
        dputs_untranslated("");
        return STATUS_CONTINUE;
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
    return STATUS_CONTINUE;
}