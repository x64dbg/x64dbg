// A real loadable DLL with no entry point or CRT dependencies. The loader-event
// test must not depend on a particular Windows DLL's changing initialization.
extern "C" __declspec(dllexport) int tracePartyModule = 1;

// An executable page makes snapshot refresh exercise newly mapped/unmapped
// target code too, not just loader notifications for a data-only image.
extern "C" __declspec(dllexport) int tracePartyFunction()
{
    return tracePartyModule;
}
