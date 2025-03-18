#include "_global.h"

GUIGUIINIT _gui_guiinit;
GUISENDMESSAGE _gui_sendmessage;
GUITRANSLATETEXT _gui_translate_text;

DBGDBGINIT _dbg_dbginit;
DBGMEMFINDBASEADDR _dbg_memfindbaseaddr;
DBGMEMREAD _dbg_memread;
DBGMEMWRITE _dbg_memwrite;
DBGDBGCMDEXEC _dbg_dbgcmdexec;
DBGMEMMAP _dbg_memmap;
DBGDBGEXITSIGNAL _dbg_dbgexitsignal;
DBGVALFROMSTRING _dbg_valfromstring;
DBGISDEBUGGING _dbg_isdebugging;
DBGISJUMPGOINGTOEXECUTE _dbg_isjumpgoingtoexecute;
DBGADDRINFOGET _dbg_addrinfoget;
DBGADDRINFOSET _dbg_addrinfoset;
DBGENCODETYPESET _dbg_encodetypeset;
DBGBPGETTYPEAT _dbg_bpgettypeat;
DBGGETREGDUMP _dbg_getregdump;
DBGGETREGDUMPAVX512 _dbg_getregdump_AVX512;
DBGVALTOSTRING _dbg_valtostring;
DBGMEMISVALIDREADPTR _dbg_memisvalidreadptr;
DBGGETBPLIST _dbg_getbplist;
DBGDBGCMDEXECDIRECT _dbg_dbgcmddirectexec;
DBGGETBRANCHDESTINATION _dbg_getbranchdestination;
DBGSENDMESSAGE _dbg_sendmessage;