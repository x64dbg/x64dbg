#ifndef ANIMATE_H
#define ANIMATE_H

bool _dbg_animatecommand(const char* command);
void _dbg_setanimateinterval(unsigned int milliseconds);
bool _dbg_isanimating();
inline void _dbg_animatestop()
{
    _dbg_animatecommand(nullptr);
}
#endif //ANIMATE_H