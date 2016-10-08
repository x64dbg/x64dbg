#include "cmd-user-database.h"
#include "database.h"
#include "console.h"
#include "comment.h"
#include "value.h"
#include "variable.h"
#include "label.h"
#include "bookmark.h"
#include "function.h"
#include "argument.h"

CMDRESULT cbInstrDbsave(int argc, char* argv[])
{
    DbSave(DbLoadSaveType::All, argc > 1 ? argv[1] : nullptr, argc > 1);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrDbload(int argc, char* argv[])
{
    if(argc <= 1)
        DbClear();
    DbLoad(DbLoadSaveType::All, argc > 1 ? argv[1] : nullptr);
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrDbclear(int argc, char* argv[])
{
    DbClear();
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCommentSet(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!CommentSet(addr, argv[2], true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error setting comment"));
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCommentDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!CommentDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error deleting comment"));
        return STATUS_ERROR;
    }
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCommentList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Comments")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Address")));
    GuiReferenceAddColumn(64, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly")));
    GuiReferenceAddColumn(10, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Comment")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    size_t cbsize;
    CommentEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "no comments"));
        return STATUS_CONTINUE;
    }
    Memory<COMMENTSINFO*> comments(cbsize, "cbInstrCommentList:comments");
    CommentEnum(comments(), 0);
    int count = (int)(cbsize / sizeof(COMMENTSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", comments()[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(comments()[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
        GuiReferenceSetCellContent(i, 2, comments()[i].text);
    }
    varset("$result", count, false);
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d comment(s) listed in Reference View\n"), count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrCommentClear(int argc, char* argv[])
{
    CommentClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all comments deleted!"));
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLabelSet(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!LabelSet(addr, argv[2], true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error setting label"));
        return STATUS_ERROR;
    }
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLabelDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!LabelDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error deleting label"));
        return STATUS_ERROR;
    }
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLabelList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Labels")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Address")));
    GuiReferenceAddColumn(64, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    size_t cbsize;
    LabelEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "no labels"));
        return STATUS_CONTINUE;
    }
    Memory<LABELSINFO*> labels(cbsize, "cbInstrLabelList:labels");
    LabelEnum(labels(), 0);
    int count = (int)(cbsize / sizeof(LABELSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", labels()[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(labels()[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
        GuiReferenceSetCellContent(i, 2, labels()[i].text);
    }
    varset("$result", count, false);
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d label(s) listed in Reference View\n"), count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrLabelClear(int argc, char* argv[])
{
    LabelClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all labels deleted!"));
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkSet(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!BookmarkSet(addr, true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to set bookmark!"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "bookmark set!"));
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!BookmarkDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to delete bookmark!"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "bookmark deleted!"));
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Bookmarks")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Address")));
    GuiReferenceAddColumn(64, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label/Comment")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    size_t cbsize;
    BookmarkEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No bookmarks found"));
        return STATUS_CONTINUE;
    }
    Memory<BOOKMARKSINFO*> bookmarks(cbsize, "cbInstrBookmarkList:bookmarks");
    BookmarkEnum(bookmarks(), 0);
    int count = (int)(cbsize / sizeof(BOOKMARKSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", bookmarks()[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(bookmarks()[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
        char comment[MAX_COMMENT_SIZE] = "";
        if(CommentGet(bookmarks()[i].addr, comment))
            GuiReferenceSetCellContent(i, 2, comment);
        else
        {
            char label[MAX_LABEL_SIZE] = "";
            if(LabelGet(bookmarks()[i].addr, label))
                GuiReferenceSetCellContent(i, 2, label);
        }
    }
    varset("$result", count, false);
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d bookmark(s) listed\n"), count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrBookmarkClear(int argc, char* argv[])
{
    LabelClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all bookmarks deleted!"));
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionAdd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    duint start = 0;
    duint end = 0;
    if(!valfromstring(argv[1], &start, false) || !valfromstring(argv[2], &end, false))
        return STATUS_ERROR;
    if(!FunctionAdd(start, end, true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to add function"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "function added!"));
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!FunctionDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to delete function"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "function deleted!"));
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Functions")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Start")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "End")));
    GuiReferenceAddColumn(64, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly (Start)")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label/Comment")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    size_t cbsize;
    FunctionEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No functions"));
        return STATUS_CONTINUE;
    }
    Memory<FUNCTIONSINFO*> functions(cbsize, "cbInstrFunctionList:functions");
    FunctionEnum(functions(), 0);
    int count = (int)(cbsize / sizeof(FUNCTIONSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", functions()[i].start);
        GuiReferenceSetCellContent(i, 0, addrText);
        sprintf_s(addrText, "%p", functions()[i].end);
        GuiReferenceSetCellContent(i, 1, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(functions()[i].start, disassembly))
            GuiReferenceSetCellContent(i, 2, disassembly);
        char label[MAX_LABEL_SIZE] = "";
        if(LabelGet(functions()[i].start, label))
            GuiReferenceSetCellContent(i, 3, label);
        else
        {
            char comment[MAX_COMMENT_SIZE] = "";
            if(CommentGet(functions()[i].start, comment))
                GuiReferenceSetCellContent(i, 3, comment);
        }
    }
    varset("$result", count, false);
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d function(s) listed\n"), count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrFunctionClear(int argc, char* argv[])
{
    FunctionClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all functions deleted!"));
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrArgumentAdd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    duint start = 0;
    duint end = 0;
    if(!valfromstring(argv[1], &start, false) || !valfromstring(argv[2], &end, false))
        return STATUS_ERROR;
    if(!ArgumentAdd(start, end, true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to add argument"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "argument added!"));
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrArgumentDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!ArgumentDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to delete argument"));
        return STATUS_ERROR;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "argument deleted!"));
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrArgumentList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Arguments")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Start")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "End")));
    GuiReferenceAddColumn(64, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly (Start)")));
    GuiReferenceAddColumn(10, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label/Comment")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    size_t cbsize;
    ArgumentEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No arguments"));
        return STATUS_CONTINUE;
    }
    Memory<ARGUMENTSINFO*> arguments(cbsize, "cbInstrArgumentList:arguments");
    ArgumentEnum(arguments(), 0);
    int count = (int)(cbsize / sizeof(ARGUMENTSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", arguments()[i].start);
        GuiReferenceSetCellContent(i, 0, addrText);
        sprintf_s(addrText, "%p", arguments()[i].end);
        GuiReferenceSetCellContent(i, 1, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(arguments()[i].start, disassembly))
            GuiReferenceSetCellContent(i, 2, disassembly);
        char label[MAX_LABEL_SIZE] = "";
        if(LabelGet(arguments()[i].start, label))
            GuiReferenceSetCellContent(i, 3, label);
        else
        {
            char comment[MAX_COMMENT_SIZE] = "";
            if(CommentGet(arguments()[i].start, comment))
                GuiReferenceSetCellContent(i, 3, comment);
        }
    }
    varset("$result", count, false);
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d argument(s) listed\n"), count);
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrArgumentClear(int argc, char* argv[])
{
    ArgumentClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all arguments deleted!"));
    return STATUS_CONTINUE;
}