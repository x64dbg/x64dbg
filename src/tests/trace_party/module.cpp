// A real loadable DLL with no entry point or CRT dependencies. The loader-event
// test must not depend on a particular Windows DLL's changing initialization.
extern "C" __declspec(dllexport) int tracePartyModule = 1;
