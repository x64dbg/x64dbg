#pragma once

#include "command.h"

bool cbInstrDbsave(int argc, char* argv[]);
bool cbInstrDbload(int argc, char* argv[]);
bool cbInstrDbclear(int argc, char* argv[]);

bool cbInstrCommentSet(int argc, char* argv[]);
bool cbInstrCommentDel(int argc, char* argv[]);
bool cbInstrCommentList(int argc, char* argv[]);
bool cbInstrCommentClear(int argc, char* argv[]);

bool cbInstrLabelSet(int argc, char* argv[]);
bool cbInstrLabelDel(int argc, char* argv[]);
bool cbInstrLabelList(int argc, char* argv[]);
bool cbInstrLabelClear(int argc, char* argv[]);

bool cbInstrBookmarkSet(int argc, char* argv[]);
bool cbInstrBookmarkDel(int argc, char* argv[]);
bool cbInstrBookmarkList(int argc, char* argv[]);
bool cbInstrBookmarkClear(int argc, char* argv[]);

bool cbInstrFunctionAdd(int argc, char* argv[]);
bool cbInstrFunctionDel(int argc, char* argv[]);
bool cbInstrFunctionList(int argc, char* argv[]);
bool cbInstrFunctionClear(int argc, char* argv[]);

bool cbInstrArgumentAdd(int argc, char* argv[]);
bool cbInstrArgumentDel(int argc, char* argv[]);
bool cbInstrArgumentList(int argc, char* argv[]);
bool cbInstrArgumentClear(int argc, char* argv[]);

bool cbInstrLoopAdd(int argc, char* argv[]);
bool cbInstrLoopDel(int argc, char* argv[]);
bool cbInstrLoopList(int argc, char* argv[]);
bool cbInstrLoopClear(int argc, char* argv[]);