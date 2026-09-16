#include <ElfBug/thread/Thread.h>

namespace ElfBug
{
    bool Thread::freeHardwareBreakpointSlot(const HardwareSlot & slot) const
    {
        (void)slot;
        return false;
    }

    bool Thread::setHardwareBreakpoint(const ptr address, const HardwareSlot slot, HardwareType type, const HardwareSize size, const bool singleshot)
    {
        (void)address;
        (void)slot;
        (void)type;
        (void)size;
        (void)singleshot;
        return false;
    }

    bool Thread::setHardwareBreakpoint(const ptr address, const HardwareSlot slot, const BreakpointCallback & cbBreakpoint, const HardwareType type, const HardwareSize size, const bool singleshot)
    {
        (void)address;
        (void)slot;
        (void)cbBreakpoint;
        (void)type;
        (void)size;
        (void)singleshot;
        return false;
    }

    bool Thread::deleteHardwareBreakpoint(const ptr address)
    {
        (void)address;
        return false;
    }
}
