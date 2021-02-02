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
#include "loop.h"
#include "debugger.h"
#include "stringformat.h"
#include "_exports.h"

bool cbInstrDbsave(int argc, char* argv[])
{
    DbSave(DbLoadSaveType::All, argc > 1 ? argv[1] : nullptr, argc > 1);
    return true;
}

bool cbInstrDbload(int argc, char* argv[])
{
    if(argc <= 1)
    {
        DebugRemoveBreakpoints();
        DbClear();
    }
    DbLoad(DbLoadSaveType::All, argc > 1 ? argv[1] : nullptr);
    DebugSetBreakpoints();
    GuiUpdateAllViews();
    return true;
}

bool cbInstrDbclear(int argc, char* argv[])
{
    DebugRemoveBreakpoints();
    DbClear();
    dputs(QT_TRANSLATE_NOOP("DBG", "Database cleared!"));
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting comment"));
        return false;
    }
    GuiUpdateAllViews();
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Error deleting comment"));
        return false;
    }
    GuiUpdateAllViews();
    return true;
}

bool cbInstrCommentList(int argc, char* argv[])
{
    auto listAuto = argc >= 2 && *argv[1] == '1';
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
        dputs(QT_TRANSLATE_NOOP("DBG", "No comments"));
        return true;
    }
    Memory<COMMENTSINFO*> comments(cbsize, "cbInstrCommentList:comments");
    CommentEnum(comments(), 0);
    int total = 0;
    for(int i = 0; i < (int)(cbsize / sizeof(COMMENTSINFO)); i++)
    {
        if(!listAuto && !comments()[i].manual)
            continue;
        GuiReferenceSetRowCount(total + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", comments()[i].addr);
        GuiReferenceSetCellContent(total, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(comments()[i].addr, disassembly))
            GuiReferenceSetCellContent(total, 1, disassembly);
        GuiReferenceSetCellContent(total, 2, comments()[i].text.c_str());
        total++;
    }
    varset("$result", total, false);
    GuiReferenceAddCommand(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Delete")), "commentdel $0");
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d comment(s) listed in Reference View\n"), total);
    GuiReferenceReloadData();
    return true;
}

bool cbInstrCommentClear(int argc, char* argv[])
{
    CommentClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "All comments deleted!"));
    return true;
}

bool cbInstrLabelSet(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    auto temporaryLabel = argc > 3;
    if(!LabelSet(addr, stringformatinline(argv[2]).c_str(), true, temporaryLabel))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error setting label"));
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Error deleting label"));
        return false;
    }
    return true;
}

bool cbInstrLabelList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Labels")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Address")));
    GuiReferenceAddColumn(100, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    std::vector<LABELSINFO> labels;
    LabelGetList(labels);
    if(labels.empty())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No labels"));
        return true;
    }
    auto count = int(labels.size());
    for(int i = 0; i < count; i++)
    {
        labels[i].addr += ModBaseFromName(labels[i].mod().c_str());
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", labels[i].addr);
        GuiReferenceSetCellContent(i, 0, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(labels[i].addr, disassembly))
            GuiReferenceSetCellContent(i, 1, disassembly);
        GuiReferenceSetCellContent(i, 2, labels[i].text.c_str());
    }
    varset("$result", count, false);
    GuiReferenceAddCommand(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Delete")), "labeldel $0");
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d label(s) listed in Reference View\n"), count);
    GuiReferenceReloadData();
    return true;
}

bool cbInstrLabelClear(int argc, char* argv[])
{
    LabelClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "All labels deleted!"));
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to set bookmark!"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Bookmark set!"));
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to delete bookmark!"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Bookmark deleted!"));
    return true;
}

bool cbInstrBookmarkList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Bookmarks")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Address")));
    GuiReferenceAddColumn(50, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly")));
    GuiReferenceAddColumn(50, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Comment")));
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
            GuiReferenceSetCellContent(i, 3, comment);
        char label[MAX_LABEL_SIZE] = "";
        if(LabelGet(bookmarks()[i].addr, label))
            GuiReferenceSetCellContent(i, 2, label);
    }
    varset("$result", count, false);
    GuiReferenceAddCommand(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Delete")), "bookmarkdel $0");
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d bookmark(s) listed\n"), count);
    GuiReferenceReloadData();
    return true;
}

bool cbInstrBookmarkClear(int argc, char* argv[])
{
    LabelClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "All bookmarks deleted!"));
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to add function"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Function added!"));
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to delete function"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Function deleted!"));
    GuiUpdateAllViews();
    return true;
}

bool cbInstrFunctionList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Functions")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Start")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "End")));
    GuiReferenceAddColumn(5, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Size")));
    GuiReferenceAddColumn(50, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label")));
    GuiReferenceAddColumn(50, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly (Start)")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Comment")));
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
        sprintf_s(addrText, ArchValue("%X", "%llX"), functions()[i].end - functions()[i].start);
        GuiReferenceSetCellContent(i, 2, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(functions()[i].start, disassembly))
            GuiReferenceSetCellContent(i, 4, disassembly);
        char label[MAX_LABEL_SIZE] = "";
        BRIDGE_ADDRINFO labelinfo;
        labelinfo.flags = flaglabel;
        if(_dbg_addrinfoget(functions()[i].start, SEG_DEFAULT, &labelinfo))
            GuiReferenceSetCellContent(i, 3, labelinfo.label);
        char comment[MAX_COMMENT_SIZE] = "";
        if(CommentGet(functions()[i].start, comment))
            GuiReferenceSetCellContent(i, 5, comment);
    }
    varset("$result", count, false);
    GuiReferenceAddCommand(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Delete")), "functiondel $0");
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d function(s) listed\n"), count);
    GuiReferenceReloadData();
    return true;
}

bool cbInstrFunctionClear(int argc, char* argv[])
{
    FunctionClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "All functions deleted!"));
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to add argument"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Argument added!"));
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
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to delete argument"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Argument deleted!"));
    GuiUpdateAllViews();
    return true;
}

bool cbInstrArgumentList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Arguments")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Start")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "End")));
    GuiReferenceAddColumn(100, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly (Start)")));
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
    GuiReferenceAddCommand(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Delete")), "argumentdel $0");
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d argument(s) listed\n"), count);
    GuiReferenceReloadData();
    return true;
}

bool cbInstrArgumentClear(int argc, char* argv[])
{
    ArgumentClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "All arguments deleted!"));
    return true;
}

bool cbInstrLoopAdd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint start = 0;
    duint end = 0;
    if(!valfromstring(argv[1], &start, false) || !valfromstring(argv[2], &end, false))
        return false;
    if(!LoopAdd(start, end, true))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to add loop"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Loop added!"));
    GuiUpdateAllViews();
    return true;
}

bool cbInstrLoopDel(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    duint depth = 0;
    if(argc >= 3 && !valfromstring(argv[2], &depth, false))
        return false;
    if(!LoopDelete(int(depth), addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to delete loop"));
        return false;
    }
    dputs(QT_TRANSLATE_NOOP("DBG", "Loop deleted!"));
    GuiUpdateAllViews();
    return true;
}

bool cbInstrLoopList(int argc, char* argv[])
{
    //setup reference view
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Loops")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Start")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "End")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Depth")));
    GuiReferenceAddColumn(2 * sizeof(duint), GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Parent")));
    GuiReferenceAddColumn(100, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Disassembly (Start)")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Label/Comment")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    size_t cbsize;
    LoopEnum(0, &cbsize);
    if(!cbsize)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No loops"));
        return true;
    }
    Memory<LOOPSINFO*> loops(cbsize, "cbInstrLoopList:loops");
    LoopEnum(loops(), 0);
    int count = (int)(cbsize / sizeof(LOOPSINFO));
    for(int i = 0; i < count; i++)
    {
        GuiReferenceSetRowCount(i + 1);
        char addrText[20] = "";
        sprintf_s(addrText, "%p", loops()[i].start);
        GuiReferenceSetCellContent(i, 0, addrText);
        sprintf_s(addrText, "%p", loops()[i].end);
        GuiReferenceSetCellContent(i, 1, addrText);
        sprintf_s(addrText, "%d", loops()[i].depth);
        GuiReferenceSetCellContent(i, 2, addrText);
        sprintf_s(addrText, "%p", loops()[i].parent);
        GuiReferenceSetCellContent(i, 3, addrText);
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE] = "";
        if(GuiGetDisassembly(loops()[i].start, disassembly))
            GuiReferenceSetCellContent(i, 4, disassembly);
        char label[MAX_LABEL_SIZE] = "";
        if(LabelGet(loops()[i].start, label))
            GuiReferenceSetCellContent(i, 5, label);
        else
        {
            char comment[MAX_COMMENT_SIZE] = "";
            if(CommentGet(loops()[i].start, comment))
                GuiReferenceSetCellContent(i, 5, comment);
        }
    }
    varset("$result", count, false);
    GuiReferenceAddCommand(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Delete")), "loopdel $0");
    dprintf(QT_TRANSLATE_NOOP("DBG", "%d loop(s) listed\n"), count);
    GuiReferenceReloadData();
    return true;
}

bool cbInstrLoopClear(int argc, char* argv[])
{
    LoopClear();
    GuiUpdateAllViews();
    dputs(QT_TRANSLATE_NOOP("DBG", "All loops deleted!"));
    return true;
}