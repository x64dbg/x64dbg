/**
 @file _global.cpp

 @brief Implements the global class.
 */

#include "_global.h"

/**
 @brief GUI functions.
 */

GUIGUIINIT _gui_guiinit;

/**
 @brief The graphical user interface sendmessage.
 */

GUISENDMESSAGE _gui_sendmessage;

/**
 @brief DBG functions.
 */

DBGDBGINIT _dbg_dbginit;

/**
 @brief The debug memfindbaseaddr.
 */

DBGMEMFINDBASEADDR _dbg_memfindbaseaddr;

/**
 @brief The debug memread.
 */

DBGMEMREAD _dbg_memread;

/**
 @brief The debug memwrite.
 */

DBGMEMWRITE _dbg_memwrite;

/**
 @brief The debug dbgcmdexec.
 */

DBGDBGCMDEXEC _dbg_dbgcmdexec;

/**
 @brief The debug memmap.
 */

DBGMEMMAP _dbg_memmap;

/**
 @brief The debug dbgexitsignal.
 */

DBGDBGEXITSIGNAL _dbg_dbgexitsignal;

/**
 @brief The debug valfromstring.
 */

DBGVALFROMSTRING _dbg_valfromstring;

/**
 @brief The debug isdebugging.
 */

DBGISDEBUGGING _dbg_isdebugging;

/**
 @brief The debug isjumpgoingtoexecute.
 */

DBGISJUMPGOINGTOEXECUTE _dbg_isjumpgoingtoexecute;

/**
 @brief The debug addrinfoget.
 */

DBGADDRINFOGET _dbg_addrinfoget;

/**
 @brief The debug addrinfoset.
 */

DBGADDRINFOSET _dbg_addrinfoset;

/**
 @brief The debug bpgettypeat.
 */

DBGBPGETTYPEAT _dbg_bpgettypeat;

/**
 @brief The debug getregdump.
 */

DBGGETREGDUMP _dbg_getregdump;

/**
 @brief The debug valtostring.
 */

DBGVALTOSTRING _dbg_valtostring;

/**
 @brief The debug memisvalidreadptr.
 */

DBGMEMISVALIDREADPTR _dbg_memisvalidreadptr;

/**
 @brief The debug getbplist.
 */

DBGGETBPLIST _dbg_getbplist;

/**
 @brief The debug dbgcmddirectexec.
 */

DBGDBGCMDEXECDIRECT _dbg_dbgcmddirectexec;

/**
 @brief The debug getbranchdestination.
 */

DBGGETBRANCHDESTINATION _dbg_getbranchdestination;

/**
 @brief The debug sendmessage.
 */

DBGSENDMESSAGE _dbg_sendmessage;
