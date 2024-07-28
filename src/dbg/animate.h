#ifndef ANIMATE_H
#define ANIMATE_H

bool dbganimatecommand(const char* command);
void _dbg_setanimateinterval(unsigned int milliseconds);
bool _dbg_isanimating();
inline void _dbg_animatestop()
{
    dbganimatecommand(nullptr);
}
#endif //ANIMATE_H