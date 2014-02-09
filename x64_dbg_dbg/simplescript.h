#ifndef _SIMPLESCRIPT_H
#define _SIMPLESCRIPT_H

#include "command.h"

//enums
enum SCRIPTBRANCHTYPE
{
    scriptnobranch,
    scriptjmp,
    scriptjnejnz,
    scriptjejz,
    scriptjbjl,
    scriptjajg,
    scriptjbejle,
    scriptjaejge
};

//structures
struct SCRIPTBP
{
    int line;
    bool silent; //do not show in GUI
};

struct SCRIPTBRANCH
{
    SCRIPTBRANCHTYPE type;
    char branchlabel[256];
};

struct LINEMAPENTRY
{
    SCRIPTLINETYPE type;
    char raw[256];
    union
    {
        char command[256];
        SCRIPTBRANCH branch;
        char label[256];
        char comment[256];
    } u;
};

//functions
void scriptload(const char* filename);
void scriptunload();
void scriptrun(int destline);
void scriptstep();
bool scriptbptoggle(int line);
bool scriptbpget(int line);
bool scriptcmdexec(const char* command);
void scriptabort();
SCRIPTLINETYPE scriptgetlinetype(int line);
void scriptsetip(int line);

#endif // _SIMPLESCRIPT_H
