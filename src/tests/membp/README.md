# memory breakpoint test fixture

This directory contains the shared executable and plugin harness for both documented `membp` behavior tests and `bpmrange` regression tests.

## Why

`SetMemoryBPX/membp` is documented to watch the whole memory region containing the supplied address, so the old report in `issue.txt` is not a valid `membp` bug by itself.

The same fixture is therefore reused for:

- positive `membp` region-behavior checks
- `bpmrange` exact-range install/hit/lifecycle regression tests

## Executable layout

`target.cpp` builds one executable/readable/writable merged PE section so code and data live in the same continuous region.

Exports used by the scripts:

- `ReadSequence`
- `WriteSequence`
- `ExecSequence`
- `ReadTarget`
- `WriteTarget`

A large padding block forces `ReadTarget` and `WriteTarget` onto a later page while keeping them in the same merged region as the code.

## Variant scripts

The directory now uses multiple scripts handled by `src/tests/run.py`:

- `test.txt` -> `membp`
- `test.write.txt` -> `membp/write`
- `test.range-read.txt` -> `membp/range-read`
- `test.range-write.txt` -> `membp/range-write`
- `test.range-execute.txt` -> `membp/range-execute`
- `test.range-delete.txt` -> `membp/range-delete`
- `test.range-reenable.txt` -> `membp/range-reenable`
- `test.range-reinit.txt` -> `membp/range-reinit`

The plugin in `plugin.cpp` provides shared assertions for installation, hits, deletion, re-enable, and reinit scenarios.
