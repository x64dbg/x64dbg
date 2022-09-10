# Breakpoint refactor regressions

These tests exercise public debugger commands and breakpoint APIs in real debug
sessions. The original nine cases use no failure injection. The PR review cases
below additionally exercise rejected engine operations using a scoped test-plugin
import override, real one-shot DLL load events, and persistent DLL unload/reload
cycles; no production test hook is used.

## Original refactor cases

| Test id (under `breakpoint_refactor/`) | Regression |
| --- | --- |
| Primary `breakpoint_refactor` test | Disabled software breakpoint is still active in `DbgGetBpList`. |
| `software-ref` | Same stale state through `BpGetFieldNumber(bpf_active)`. |
| `hardware-list`, `hardware-ref` | Disabled hardware breakpoint is still active through either API. |
| `memory-list`, `memory-ref` | Disabled memory breakpoint is still active through either API. |
| `delete-disabled-address` | Deleting a disabled memory breakpoint by address attempts a second engine removal and reports failure. |
| `delete-disabled-name` | Same failure through the breakpoint-name lookup path. |
| `enable-failure` | Bulk enable marks a memory breakpoint enabled before the engine accepts it. |

The active-state cases check an enabled breakpoint, disable it, require both
`enabled=0` and `active=0`, then re-enable it and require both flags to be set
again. List and reference checks use separate sessions because querying
`bpf_active` can modify the stored flag in the pre-refactor implementation.

Memory cases use a dedicated allocation, not a module or shared code page. The
failed-enable case disables the breakpoint, frees its allocation, verifies the
address is unmapped, and requires bulk enable to fail **without** setting the
database's enabled flag. Explicit command-result assertions distinguish expected
engine rejection from unexpected script failure. Deletion cases also verify the
database entry is gone.

## Run

```sh
cmake --build build --target headless x64dbg_tests
py src/tests/run.py --arch x64 \
  breakpoint_refactor \
  breakpoint_refactor/software-ref \
  breakpoint_refactor/hardware-list \
  breakpoint_refactor/hardware-ref \
  breakpoint_refactor/memory-list \
  breakpoint_refactor/memory-ref \
  breakpoint_refactor/delete-disabled-address \
  breakpoint_refactor/delete-disabled-name \
  breakpoint_refactor/enable-failure
```

They are also included in `py src/tests/run.py --arch x64`.

## Original refactor red/green comparison

On x64 with TitanEngine, the identical test sources and plugin were tested with:

- **Before:** `be34ad21` (parent of this refactor), **0/9 passed**. All nine failed
  with `reason=assert_failed`, not a timeout, crash, or setup failure.
- **After:** the original refactor (`3a8d755b`), **9/9 passed**. At that point,
  the complete x64 suite passed **91/91**, including the two unloaded-module tests.

For the baseline build, only the four production files changed by the refactor
were restored from `be34ad21`, leaving these tests in place. From this refactor
commit, with clean production files, the comparison can be repeated in Bash:

```sh
(
  refactor=$(git rev-parse HEAD)
  files=(src/dbg/breakpoint.cpp src/dbg/breakpoint.h
         src/dbg/commands/cmd-breakpoint-control.cpp src/dbg/debugger.cpp)
  git diff --quiet HEAD -- "${files[@]}" || exit 1
  trap 'git restore --source="$refactor" --worktree -- "${files[@]}" &&
        cmake --build build --target dbg' EXIT
  git restore --source=be34ad21 --worktree -- "${files[@]}" || exit 1
  cmake --build build --target dbg || exit 1
  tests=$(py src/tests/run.py --arch x64 --list |
          tr -d '\r' | grep '^breakpoint_refactor' | grep -v '/review-' | tr '\n' ' ')
  py src/tests/run.py --arch x64 $tests
  test $? -eq 1  # All nine tests must report assertion failures.
)
```

The EXIT trap restores and rebuilds the refactor even on failure. The parent
assertions were:

- Six active-state tests: `<list|ref> <sw|hw|mem> breakpoint active=1, expected 0`.
- Both deletion tests: `command 'bpmc ...' returned failure, expected success`.
- Failed enable: `list mem breakpoint enabled=1, expected 0 ($page)`.

## PR #3994 review regressions

These 27 additional cases reproduce the two review findings on
[PR #3994](https://github.com/x64dbg/x64dbg/pull/3994):

- [Premature state changes on removal failure](https://github.com/x64dbg/x64dbg/pull/3994#discussion_r4305028711).
- [DLL/exception active-state API disagreement](https://github.com/x64dbg/x64dbg/pull/3994#discussion_r4305028713).

| Variants (under `breakpoint_refactor/`) | Cases | Checks |
| --- | ---: | --- |
| `review-disable-sw-{address,name,all}` | 3 | Failed software disable retains enabled/active flags and can be retried. |
| `review-disable-hw-{address,all}` | 2 | Same, including preservation of the hardware slot. |
| `review-disable-mem-all` | 1 | Same for bulk memory disable. |
| `review-delete-{sw,hw,mem}-{address,name,all}` | 9 | Failed deletion retains the database entry; retry removes both engine and database entries. |
| `review-{disable,delete}-dll-{name,all}` | 4 | Genuine DLL engine-removal failure during a consumed one-shot load callback retains the pending database entry and reports failure. The generic handler then completes normal one-shot cleanup. |
| `review-delete-disabled-dll-{name,all}` | 2 | Deleting an inactive DLL breakpoint succeeds and removes its database entry. |
| `review-{dll,exception}-state` and `review-{dll,exception}-state-all` | 4 | Both APIs report enabled/active, then disabled/inactive, then enabled/active across single and bulk toggles. |
| `review-{dll,exception}-reload` | 2 | Database reload preserves enabled/active and disabled/inactive state consistently through both APIs. |

### Removal failure isolation

For software, hardware and memory cases, the plugin replaces exactly one named
TitanEngine delay-import slot in the debugger module while the debuggee is
paused. The substitute returns failure without performing the removal. A scoped
object restores the original pointer and page protection before any state or
retry assertion, and also restores them on early return. No engine code is
patched and no engine library or submodule is changed.

The tests require exactly one intercepted call and verify the actual breakpoint
remains installed using raw `ReadProcessMemory` INT3 bytes, the thread's hardware
debug registers (including the address), or `VirtualQueryEx` PAGE_GUARD protection.
They then require the database entry, flags and slot to be unchanged. With the
import restored, repeating the same command must succeed and remove the actual
engine breakpoint. The disable tests also require an inactive retained entry;
the delete tests require no remaining entry.

The DLL cases need no import override: `dbghandledllbreakpoint` consumes the
one-shot engine entry before the load notification callback and generic handler.
A reentrant remove command therefore fails naturally. The tests verify it does
not prematurely alter the still-pending database entry. These checks concern
per-breakpoint state, not rollback of earlier successful entries in a bulk command
or rollback of partial side effects inside an engine implementation.

### Run and verified results

```sh
cmake --build build --target headless x64dbg_tests
tests=$(py src/tests/run.py --arch x64 --list |
        tr -d '\r' | grep '^breakpoint_refactor/review-' | tr '\n' ' ')
py src/tests/run.py --arch x64 $tests
# Optional alternate engine:
py src/tests/run.py --arch x64 --engine GleeBug $tests
```

The identical 27 review tests were run with production debugger sources restored
from reviewed commit `3a8d755b`: **0/27 passed**, all with `reason=assert_failed`.
The failures were lost database entries, cleared enabled/active flags, incorrect
success reporting, DLL/exception API disagreement, or lost activity on reload;
none were setup failures, crashes or timeouts. The fixes were saved separately
and restored/rebuilt through an EXIT trap after the baseline run.

After the fixes: **27/27 passed** with both x64 TitanEngine and x64 GleeBug.
The full x64 suites passed **118/118 on both engines**.

The initial local GleeBug run used an outdated checkout (`6d37c95`) and failed
both `bp_unloaded_module` variants, including against unchanged `3a8d755b`.
Development already records the fix in `b2b68b8`. Synchronizing the GleeBug
submodule to that recorded revision and running `cmake --build build` resolved
both failures; the full GleeBug suite then passed **118/118**. No GleeBug gitlink
change was needed, and the local TitanEngine checkout was left unchanged.

Evidence is retained locally in `build/pr3994-red-final/`,
`build/pr3994-full-green/` and `build/pr3994-gleebug-development/`, with summary
logs alongside those directories. The GleeBug rebuild log is
`build/pr3994-engine-rebuild.log`; earlier stale-runtime results are retained in
`build/pr3994-gleebug-full-green/` and `build/pr3994-gleebug-baseline/`.
x32 was not validated locally.

## Follow-up PR #3994 review regressions

The two older findings above were already fixed in `95ccdabc`. Nine new cases
cover the remaining review findings:

- [Persistent DLL registrations incorrectly marked inactive on module unload](https://github.com/x64dbg/x64dbg/pull/3994#discussion_r4306854694).
- [Failed hardware/memory creation leaves an active phantom database entry](https://github.com/x64dbg/x64dbg/pull/3994#discussion_r4306854698).

| Variants (under `breakpoint_refactor/`) | Cases | Checks |
| --- | ---: | --- |
| `review-create-{hw,mem,memrange}-failure` | 3 | Reject exactly one engine installation through the scoped import override. Require command failure and no database entry; restore the import, retry, verify enabled/active state and actual engine installation, then delete successfully. |
| `review-dll-unload-state`, `review-dll-unload-state-ref` | 2 | Persistent load-only and unload-only DLL registrations remain enabled/active through both APIs after the module unloads. |
| `review-dll-unload-{disable,delete}-{name,all}` | 4 | After unloading the module, disable/delete persistent load-only or load/unload registrations through single/bulk commands. Require the next run to reach the post-reload marker without another DLL breakpoint event. |

The creation fix removes the newly inserted database entry on engine rejection,
matching software-breakpoint creation. The unload fix only marks address-based
breakpoints inactive; DLL/exception registrations are not tied to module mappings.
The creation tests use all-access memory breakpoints so raw PAGE_GUARD protection
can verify actual installation independently of database flags. The DLL scripts
explicitly continue past the automatic one-shot DllMain breakpoint on load.

### Run and verified results

```sh
cmake --build build --target headless x64dbg_tests
tests=$(py src/tests/run.py --arch x64 --list |
        tr -d '\r' | grep -E '^breakpoint_refactor/review-(create-|dll-unload-)' |
        tr '\n' ' ')
py src/tests/run.py --arch x64 $tests
py src/tests/run.py --arch x64 --engine GleeBug $tests
```

With production sources restored to `95ccdabc`, the identical nine new tests
passed **0/9**, all with `reason=assert_failed`: three phantom database entries,
two incorrect active flags, and four unexpected DLL events after disable/delete.
The production fix was saved as a patch, then restored and rebuilt through an
EXIT trap. Evidence: `build/pr3994-review-final-red/` and its adjacent summary log.

After the fixes, the full x64 suites passed **127/127 on both TitanEngine and
GleeBug**, including all 45 breakpoint-refactor cases and the 27 already-fixed
review regressions. Evidence: `build/pr3994-review-full-green/` and
`build/pr3994-review-gleebug-green/`, with summary logs alongside each directory.
x32 was not validated locally.
