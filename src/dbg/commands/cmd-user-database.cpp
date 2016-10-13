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

bool cbInstrDbsave(int argc, char* argv[])
{
    DbSave(DbLoadSaveType::All, argc > 1 ? argv[1] : nullptr, argc > 1);
    return true;
}

bool cbInstrDbload(int argc, char* argv[])
{
    if(argc <= 1)
        DbClear();
    DbLoad(DbLoadSaveType::All, argc > 1 ? argv[1] : nullptr);
    GuiUpdateAllViews();
    return true;
}

bool cbInstrDbclear(int argc, char* argv[])
{
    DbClear();
    GuiUpdateAllViews();
    return true;
}

bool cbInstrCommentSet(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!CommentSet(addr, argv[2], true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error setting comment"));
        return false;
    }
    return true;
}

bool cbInstrCommentDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!CommentDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error deleting comment"));
        return false;
    }
    GuiUpdateAllViews();
    return true;
}

bool cbInstrCommentList(int argc, char* argv[])
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
        return true;
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
    return true;
}

bool cbInstrCommentClear(int argc, char* argv[])
{
    CommentClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all comments deleted!"));
    return true;
}

bool cbInstrLabelSet(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!LabelSet(addr, argv[2], true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error setting label"));
        return false;
    }
    GuiUpdateAllViews();
    return true;
}

bool cbInstrLabelDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!LabelDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "error deleting label"));
        return false;
    }
    return true;
}

bool cbInstrLabelList(int argc, char* argv[])
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
        return true;
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
    return true;
}

bool cbInstrLabelClear(int argc, char* argv[])
{
    LabelClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all labels deleted!"));
    return true;
}

bool cbInstrBookmarkSet(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!BookmarkSet(addr, true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to set bookmark!"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "bookmark set!"));
    return true;
}

bool cbInstrBookmarkDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!BookmarkDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to delete bookmark!"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "bookmark deleted!"));
    return true;
}

bool cbInstrBookmarkList(int argc, char* argv[])
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
        return true;
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
    return true;
}

bool cbInstrBookmarkClear(int argc, char* argv[])
{
    LabelClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all bookmarks deleted!"));
    return true;
}

bool cbInstrFunctionAdd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint start = 0;
    duint end = 0;
    if(!valfromstring(argv[1], &start, false) || !valfromstring(argv[2], &end, false))
        return false;
    if(!FunctionAdd(start, end, true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to add function"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "function added!"));
    GuiUpdateAllViews();
    return true;
}

bool cbInstrFunctionDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!FunctionDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to delete function"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "function deleted!"));
    GuiUpdateAllViews();
    return true;
}

bool cbInstrFunctionList(int argc, char* argv[])
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
        return true;
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
    return true;
}

bool cbInstrFunctionClear(int argc, char* argv[])
{
    FunctionClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all functions deleted!"));
    return true;
}

bool cbInstrArgumentAdd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint start = 0;
    duint end = 0;
    if(!valfromstring(argv[1], &start, false) || !valfromstring(argv[2], &end, false))
        return false;
    if(!ArgumentAdd(start, end, true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to add argument"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "argument added!"));
    GuiUpdateAllViews();
    return true;
}

bool cbInstrArgumentDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!ArgumentDelete(addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "failed to delete argument"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "argument deleted!"));
    GuiUpdateAllViews();
    return true;
}

bool cbInstrArgumentList(int argc, char* argv[])
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
        return true;
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
    return true;
}

bool cbInstrArgumentClear(int argc, char* argv[])
{
    ArgumentClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "all arguments deleted!"));
    return true;
}