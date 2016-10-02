#include "cmd-variables.h"
#include "console.h"
#include "variable.h"
#include "value.h"

CMDRESULT cbInstrVar(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    char arg2[deflen] = ""; //var value (optional)
    if(argc > 2)
        strcpy_s(arg2, argv[2]);
    duint value = 0;
    int add = 0;
    if(*argv[1] == '$')
        add++;
    if(valfromstring(argv[1] + add, &value))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "invalid variable name \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    if(!valfromstring(arg2, &value))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "invalid value \"%s\"\n"), arg2);
        return STATUS_ERROR;
    }
    if(!varnew(argv[1], value, VAR_USER))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "error creating variable \"%s\"\n"), argv[1]);
        return STATUS_ERROR;
    }
    else
    {
        if(value > 15)
#ifdef _WIN64
            dprintf_untranslated("%s=%llX (%llud)\n", argv[1], value, value);
#else //x86
            dprintf_untranslated("%s=%X (%ud)\n", argv[1], value, value);
#endif //_WIN64
        else
#ifdef _WIN64
            dprintf_untranslated("%s=%llX\n", argv[1], value);
#else //x86
            dprintf_untranslated("%s=%X\n", argv[1], value);
#endif //_WIN64
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVarDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    if(!vardel(argv[1], false))
        dprintf(QT_TRANSLATE_NOOP("DBG", "could not delete variable \"%s\"\n"), argv[1]);
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "deleted variable \"%s\"\n"), argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrVarList(int argc, char* argv[])
{
    int filter = 0;
    if(argc > 1)
    {
        if(!_stricmp(argv[1], "USER"))
            filter = VAR_USER;
        else if(!_stricmp(argv[1], "READONLY"))
            filter = VAR_READONLY;
        else if(!_stricmp(argv[1], "SYSTEM"))
            filter = VAR_SYSTEM;
    }

    size_t cbsize = 0;
    if(!varenum(0, &cbsize))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "no variables!"));
        return STATUS_CONTINUE;
    }
    Memory<VAR*> variables(cbsize, "cbInstrVarList:variables");
    if(!varenum(variables(), 0))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error listing variables!"));
        return STATUS_ERROR;
    }

    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Variables")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Value (Hex)"))); //The GUI only follows address in column 0
    GuiReferenceAddColumn(30, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Variable")));
    GuiReferenceAddColumn(3 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Value (Decimal)")));
    GuiReferenceAddColumn(20, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Property")));
    int varcount = (int)cbsize / sizeof(VAR);
    int realvarcount = 0;
    GuiReferenceSetRowCount(0);
    for(int i = 0; i < varcount; i++)
    {
        char value[32];
        if(variables()[i].alias.length())
            continue;
        if(variables()[i].type != VAR_HIDDEN)
        {
            GuiReferenceSetRowCount(realvarcount + 1);
            GuiReferenceSetCellContent(realvarcount, 1, variables()[i].name.c_str());
#ifdef _WIN64
            sprintf_s(value, "%llX", variables()[i].value.u.value);
            GuiReferenceSetCellContent(realvarcount, 0, value);
            sprintf_s(value, "%llu", variables()[i].value.u.value);
            GuiReferenceSetCellContent(realvarcount, 2, value);
#else //x86
            sprintf_s(value, "%X", variables()[i].value.u.value);
            GuiReferenceSetCellContent(realvarcount, 0, value);
            sprintf_s(value, "%u", variables()[i].value.u.value);
            GuiReferenceSetCellContent(realvarcount, 2, value);
#endif //_WIN64
            const char* szType;
            switch(variables()[i].type)
            {
            case VAR_USER:
                szType = QT_TRANSLATE_NOOP("DBG", "User Variable");
                break;
            case VAR_SYSTEM:
                szType = QT_TRANSLATE_NOOP("DBG", "System Variable");
                break;
            case VAR_READONLY:
                szType = QT_TRANSLATE_NOOP("DBG", "Read Only Variable");
                break;
            default://other variables
                szType = QT_TRANSLATE_NOOP("DBG", "System Variable");
                break;
            }
            GuiReferenceSetCellContent(realvarcount, 3, GuiTranslateText(szType));
            realvarcount++;
        }
    }
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}