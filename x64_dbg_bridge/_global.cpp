#include "_global.h"

//hInst GUI/DBG
HINSTANCE hInstGui;
HINSTANCE hInstDbg;

//GUI functions
GUIGUIINIT _gui_guiinit;
GUIDISASSEMBLEAT _gui_disassembleat;
GUISETDEBUGSTATE _gui_setdebugstate;
GUIADDLOGMESSAGE _gui_addlogmessage;
GUILOGCLEAR _gui_logclear;
GUIUPDATEREGISTERVIEW _gui_updateregisterview;

//DBG functions
DBGDBGINIT _dbg_dbginit;
DBGMEMFINDBASEADDR _dbg_memfindbaseaddr;
DBGMEMREAD _dbg_memread;
DBGDBGCMDEXEC _dbg_dbgcmdexec;
DBGMEMMAP _dbg_memmap;
DBGDBGEXITSIGNAL _dbg_dbgexitsignal;
DBGVALFROMSTRING _dbg_valfromstring;
DBGISDEBUGGING _dbg_isdebugging;
DBGISJUMPGOINGTOEXECUTE _dbg_isjumpgoingtoexecute;
DBGADDRINFOGET _dbg_addrinfoget;
DBGADDRINFOSET _dbg_addrinfoset;
DBGBPGETTYPEAT _dbg_bpgettypeat;
DBGGETREGDUMP _dbg_getregdump;
DBGVALTOSTRING _dbg_valtostring;
