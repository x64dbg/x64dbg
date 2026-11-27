# Full-width handle enumeration

The target keeps 17,000 events open and stops at `HandlesReady`. The plugin
checks that `EnumHandles` returns the exported handle above 0xFFFF and that
the returned value can be duplicated from the debuggee.

This detects truncation of handle values regardless of the PID assigned by
Windows. When Windows assigns a PID above 0xFFFF, it also covers the PID
truncation that previously left the Handles view empty. The PID is logged
so that coverage is visible without churning processes to force a high PID.
