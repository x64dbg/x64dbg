#pragma once

#include "command.h"

CMDRESULT cbInstrDbsave(int argc, char* argv[]);
CMDRESULT cbInstrDbload(int argc, char* argv[]);
CMDRESULT cbInstrDbclear(int argc, char* argv[]);

CMDRESULT cbInstrCommentSet(int argc, char* argv[]);
CMDRESULT cbInstrCommentDel(int argc, char* argv[]);
CMDRESULT cbInstrCommentList(int argc, char* argv[]);
CMDRESULT cbInstrCommentClear(int argc, char* argv[]);

CMDRESULT cbInstrLabelSet(int argc, char* argv[]);
CMDRESULT cbInstrLabelDel(int argc, char* argv[]);
CMDRESULT cbInstrLabelList(int argc, char* argv[]);
CMDRESULT cbInstrLabelClear(int argc, char* argv[]);

CMDRESULT cbInstrBookmarkSet(int argc, char* argv[]);
CMDRESULT cbInstrBookmarkDel(int argc, char* argv[]);
CMDRESULT cbInstrBookmarkList(int argc, char* argv[]);
CMDRESULT cbInstrBookmarkClear(int argc, char* argv[]);

CMDRESULT cbInstrFunctionAdd(int argc, char* argv[]);
CMDRESULT cbInstrFunctionDel(int argc, char* argv[]);
CMDRESULT cbInstrFunctionList(int argc, char* argv[]);
CMDRESULT cbInstrFunctionClear(int argc, char* argv[]);

CMDRESULT cbInstrArgumentAdd(int argc, char* argv[]);
CMDRESULT cbInstrArgumentDel(int argc, char* argv[]);
CMDRESULT cbInstrArgumentList(int argc, char* argv[]);
CMDRESULT cbInstrArgumentClear(int argc, char* argv[]);


