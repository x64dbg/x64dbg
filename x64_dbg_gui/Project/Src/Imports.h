#ifndef IMPORTS_H
#define IMPORTS_H

#ifdef BUILD_LIB
    #include "..\..\..\x64_dbg_bridge\bridgemain.h"
#else
    #include "NewTypes.h"
    void stubReadProcessMemory(byte_t* dest, uint_t va, uint_t size);
#endif


#endif // IMPORTS_H
