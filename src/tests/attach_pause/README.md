# attach_pause

Regression test for pausing an attached debuggee.

The Python driver starts `attach_pause.exe` (a message pump main thread plus
parked worker threads), then sends `attach .<pid>` and `pause` through the
headless interactive command handler. This uses `DbgCmdExec` like the GUI, so
the attach does not pause the debuggee.

After the attach settles, the driver makes a worker thread emit
`OutputDebugString` and block forever, which turns that worker into the
debugger's active thread. The `pause` command must interrupt the debuggee
anyway by targeting the main thread instead of the (parked) active thread. The
driver asserts that the debugger reaches the paused state within a timeout.

The `breakin` variant runs the target in block mode, where every thread
(including the main thread) blocks forever. The first pause request must not
interrupt anything; repeated pause requests after a few seconds must fall
back to spawning a break-in thread and pause the debuggee. The retry phase sends
up to four requests, 2.5 seconds apart, within the original ten-second wait budget.
This tolerates late worker-thread events resetting the debugger's two-second
no-event interval without hiding a missing pause or increasing the timeout.

## CI late-event regression

The x64 TitanEngine job in [run 39414724842](https://github.com/x64dbg/x64dbg/actions/runs/39414724842/job/118602322143)
failed only `attach_pause/breakin` (`167/168` passed). Its captured output includes
late worker-thread creation and a duplicate `SetBPX` error instead of a break-in
thread: the second request took the normal pause path, then no further request
was sent. The other three architecture/engine test jobs passed.

`test_driver.py` checks the driver with mocked process/state responses: a late
event requires a third pause request, never pausing must still fail within the
retry budget, and pausing on the first request must still fail. Against the old
driver, the late-event check fails and both controls pass; all three pass with
bounded retries. CI runs these checks before the real headless suite.

```sh
py -m unittest discover -s src/tests/attach_pause -p test_driver.py -q
py src/tests/run.py --arch x64 attach_pause
py src/tests/run.py --arch x64 --engine GleeBug attach_pause
```

Local red/green evidence: `build/pr3994-ci-driver-{red,green}.log`; downloaded
failing CI artifacts: `build/pr3994-ci-artifacts/attach_pause__breakin/`.
The fixed break-in test also passed **10/10 repeated x64 runs** (five per engine),
with artifacts and logs under `build/pr3994-ci-breakin-stress-*`.
