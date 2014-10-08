#include "_global.h"
#include "log.h"

log::log(void)
{
}

log::~log(void)
{
    GuiAddLogMessage(message.str().c_str());
}

