#pragma once

static const char* guimsg2str(GUIMSG msg)
{
    switch(msg)
    {
#define GUIMSG_NAME(msg) case msg: return #msg;
        GUIMSG_LIST(GUIMSG_NAME)
#undef GUIMSG_NAME
    }
    return "<unknown>";
}

static const char* dbgstate2str(DBGSTATE s)
{
    switch(s)
    {
    case initialized:
        return "initialized";
    case paused:
        return "paused";
    case running:
        return "running";
    case stopped:
        return "stopped";
    }
    return "<unknown>";
}