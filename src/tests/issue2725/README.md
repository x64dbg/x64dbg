# issue2725

Regression test for x64dbg issue #2725.

The script enables `HardcoreThreadSwitchWarning`, starts the debuggee, stops it,
and starts it again. `check.py` then asserts that the log does not contain a
thread-switch warning from the restart.
