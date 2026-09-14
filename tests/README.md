# Run-to-party tests

Run the hermetic primitive tests with:

```text
python tests/run_to_party_test.py
python tests/trace_filter_test.py
```

The trace-filter runner compiles the production startup, pending-record filter, fast-path dispatch, party-step callbacks, and Pause/temporary-breakpoint functions against test doubles. It covers an excluded initial instruction (including a direct cross-party first step), pre-existing recordings, compression-state reset, failed startup, both skip loops, cancelled late completions in either party, first-instruction run-mode arming for all four wrappers, included-call stepping, setup fallback, fast-run Pause, and repeated Pause during a blocked syscall. Pause tests require deletion before break-in, cancellation of late installations, cleanup after resume failure, and preservation of existing/replaced user breakpoints.

The run-to-party runner compiles the unchanged `src/dbg/runtoparty.cpp` against a simulated engine in a temporary directory. It requires `clang++` (or pass `--compiler`). It checks target/private-memory coverage, module-boundary splitting, process-wide hits, exclusive operation ownership, partial-setup rollback, user-memory-breakpoint and guard-page rejection, query failures, module-change fallback with the original step mode, cancellation, and callback reentrancy.

These hermetic tests do not exercise real page protections, engine trap-flag handling, or Qt. The plugin-free [trace_party E2E bundle](../src/tests/trace_party/README.md) now covers the main tracing/run-to-party paths on actual TitanEngine/GleeBug processes. The following remains a broader validation checklist (including UI and race cases not yet automated):

1. Open the trace dialog. The new checkbox must be unchecked and disabled for All Modules, enabled for User Only/System Only, and cleared when selecting All Modules again.
2. Trace through a long excluded-module loop with the checkbox off/on. Included instruction addresses, conditions, logs, and counters should agree for a single-threaded Trace Into target with stable mappings; run mode should avoid a debug event per excluded instruction.
3. Start System Only from user code with Record trace checked, then repeat with recording already active. No newly recorded starting user instruction should appear (existing file history must remain intact). Repeat User Only from system code and both Trace Into/Trace Over. Also test a first instruction that transfers directly into the selected party.
4. Include a system-to-user callback and an exception handler in the target. User Only + Trace Into must catch the callback/handler in both modes. Verify pending trace-record changes are finalized at the transition, not after the excluded loop.
5. Exercise Trace Over too. Included calls must remain stepped over. While the fast path is running excluded code, first entry into the selected party (including a callback) is intentional and can differ from individually stepping over excluded calls.
6. Test `RunUser`, `RunSystem`, `RunToParty 0`, `RunToParty 1`, `rtu`, and `rts`. Test `StepUserInto` and `StepSystemInto` for distinct aliases. Invalid parties/modes must fail without resuming or leaving breakpoints.
7. During both a long excluded-code step loop and a fast-path run, hit Pause (repeat after two seconds for a blocked syscall), a software/hardware breakpoint, an exception configured to break, and process exit. Verify no temporary memory breakpoints or stale callbacks survive. Repeat a run/trace afterward.
8. With an enabled user memory breakpoint or a target guard page, a trace must fall back to stepping; a standalone RunToParty must fail without removing the existing breakpoint/page protection. Test a debugger backend that rejects setup partway through as well.
9. Load/unload a DLL during a fast-path run. It must discard the snapshot and step until the target party is reached. Verify initial entry into the newly loaded module is observed.
10. With two threads, confirm that a selected-party hit on either thread can resume tracing. This process-wide behavior is documented and intentional; it is not thread-isolated stepping.

Run mode deliberately uses a snapshot of executable pages. Dynamically allocating or making memory executable without a DLL event can escape that snapshot; use the default stepping mode for such targets.
