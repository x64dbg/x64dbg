#include "log.h"
#include "_global.h"

log::log(void)
{
}


log::~log(void)
{
    GuiAddLogMessage(message.str().c_str());

}
