#ifndef _SIMPLESCRIPT_H
#define _SIMPLESCRIPT_H

#include "command.h"

//structures
struct SCRIPTBP
{
    int line;
    bool silent; //do not show in GUI
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
void scriptreset();
bool scriptgetbranchinfo(int line, SCRIPTBRANCH* info);
void scriptlog(const char* msg);
DWORD WINAPI scriptLoadSync(void* filename); // Load script synchronized
DWORD WINAPI scriptRunSync(void* arg);

//script commands


#endif // _SIMPLESCRIPT_H
