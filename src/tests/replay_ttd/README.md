# Deterministic TTD replay fixture

`replay_ttd.exe` is the trace target. It deliberately contains:

- three worker threads with deterministic ordering;
- 25 calls to `ReplayTtdKnownFunction`;
- exported state in `gReplayTtdState`;
- repeated reads and writes with known values;
- dynamic load and unload of `replay_ttd_helper.dll`;
- the handled exception `0xE0424242`; and
- exported `ReplayTtdMilestone` calls around each phase.

The automated test reads traces from:

```text
%X64DBG_TTD_FIXTURE_DIR%/x64/replay_ttd.run
%X64DBG_TTD_FIXTURE_DIR%/x32/replay_ttd.run
```

When the variable is unset it uses `build/replay-ttd-fixtures` in the repository.
The checked manifest records recorder, binary, trace, extent, and milestone data.
The gate also exercises a deterministic navigation transition matrix: alternating
forward/reverse steps at the first position; persistent code/data breakpoint
hits approached and left in both directions; forward/reverse run and step-over
transitions around the handled exception; standard repeated step-over and
current-thread step-over at the loader call shared by multiple thread
initializations; rejected movement beyond both trace
boundaries; pseudo-exit reverse/forward navigation; interruption; and repeated
session teardown.

The current fixtures were recorded from an elevated command prompt with:

```bat
TTD.exe -acceptEula -noUI -out <fixture> -passThroughExit -launch <replay_ttd.exe>
```

Record the x64 target with the amd64 recorder and the x32 target with the x86
recorder. Preserve each architecture's EXE, helper DLL, and PDB files beside the
test runtime so symbol expressions remain reproducible.
