// HACK: pretend we are in the same environment as a plugin
//#define PLUG_IMPEXP
#ifdef BUILD_DBG
#undef BUILD_DBG
#endif // BUILD_DBG

#include "_plugins.h"
