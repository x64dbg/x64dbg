# x64dbg tests

This is the convention-based automated test tree for headless x64dbg regression tests.

## Core concepts

Each test is a **bundle** identified by its folder name under `src/tests/`.

For a test named `<rel>`:

- source folder: `src/tests/<rel>/`
- debuggee executable output: `bin/<arch>/tests/<rel>.exe`
- runtime directory: `bin/<arch>/tests/<rel>/`
- optional plugin output: `bin/<arch>/tests/<rel>/<rel>.dp32|dp64`
- test id: `<rel>`

The build system derives those paths and names centrally in `src/tests/CMakeLists.txt` via `x64dbg_add_test(...)`.
Per-test folders do **not** contain their own `CMakeLists.txt` anymore.

## Source layout

Enabled tests are declared centrally in `src/tests/CMakeLists.txt`.

A standard test folder looks like this:

```text
src/tests/<rel>/
  target.cpp
  optional plugin.cpp
  test.txt
  optional test.<variant>.txt
  optional driver.py
  optional driver.<variant>.py
  optional check.py
  optional check.<variant>.py
  optional absolute_cf
  optional README.md
```

Conventions:

- `target.cpp` is the debuggee built as `<rel>.exe`
- `plugin.cpp` is optional; if present it is built automatically as `<rel>.dp32|dp64`
- `test.txt` is the primary script for test id `<rel>`
- `test.<variant>.txt` creates test id `<rel>/<variant>`
- `driver.py` and `driver.<variant>.py` are optional Python drivers for tests that need custom process setup before headless starts
- `check.py` and `check.<variant>.py` are optional fallback validators used by `run.py`
- `absolute_cf` makes `run.py` pass the script path to `-cf` as an absolute path

Examples:

- `src/tests/membp/test.txt` -> `membp`
- `src/tests/membp/test.write.txt` -> `membp/write`

### Driver tests

When a driver is present, `run.py` launches it instead of launching headless directly. The driver receives `--headless`, `--debuggee`, `--script`, `--runtime-dir`, `--userdir`, `--log`, `--artifacts-dir`, `--engine`, `--timeout`, and optionally `--no-console-window`. It is responsible for starting and stopping child processes, writing the canonical `FINAL` line to `--log`, and returning a nonzero exit code on failure. Driver tests that need plugins must load them when starting headless.

## Runtime layout

Build output is placed under:

```text
bin/<arch>/tests/
  <rel>.exe
  <rel>/
    test.txt
    test.<variant>.txt
    <rel>.dp32 | <rel>.dp64
```

The runner discovers tests from `src/tests/test*.txt`, then expects the matching built runtime files in `bin/<arch>/tests/`.

## Adding a new test

1. Create a new folder:

   ```text
   src/tests/<name>/
   ```

2. Add a debuggee source as `target.cpp`.

3. Add a script as `test.txt`.

4. If the test needs a plugin, add `plugin.cpp`.

5. Register the test in `src/tests/CMakeLists.txt`:

   - standard case: add the folder name to `X64DBG_STANDARD_TESTS`
   - special case: add an explicit `x64dbg_add_test(NAME <name> ...)` call with the required options

6. Build and run it:

   ```powershell
   cmake --build build --target x64dbg_tests
   py src/tests/run.py --arch x64 <name>
   ```

### Standard test example

Folder:

```text
src/tests/my_test/
  target.cpp
  plugin.cpp
  test.txt
```

Registration in `src/tests/CMakeLists.txt`:

```cmake
list(APPEND X64DBG_STANDARD_TESTS my_test)
```

### Special-case test example

Use an explicit declaration when the debuggee or plugin needs unusual compile or link flags:

```cmake
x64dbg_add_test(
    NAME my_special_test
    TARGET_LINK_LIBRARIES kernel32
    TARGET_LINK_OPTIONS
        "LINKER:/entry:start"
)
```

## Plugin conventions

Test plugins should use the injected `X64DBG_TEST_NAME` compile definition instead of hardcoding their runtime name.

Example:

```cpp
strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), X64DBG_TEST_NAME, _TRUNCATE);
```

This keeps these concepts aligned:

- test folder name
- test id
- plugin file name
- plugin runtime name

### Keep test orchestration in the script

The test script should show the debugger operations being exercised. Put normal x64dbg commands, their ordering, and lifecycle operations such as setup, enable/disable, and cleanup directly in `test.txt`. This makes failures reproducible from the script and keeps command coverage visible during review.

Use plugin commands only for behavior that the script command language cannot express, or when the context of plugin execution is itself part of the test. Plugin helpers should be narrow and named after the specific API action or assertion they provide.

In particular:

- do not hide a sequence of ordinary debugger commands inside one opaque plugin command merely for convenient orchestration
- do not call `DbgCmdExecDirect` from a plugin helper when the same commands can be written in `test.txt`
- prefer separate API mutation and assertion helpers so the surrounding command sequence remains visible
- if commands must run from a callback, reentrant path, or another special API context, keep them in the plugin and document why that context is required

For example, a breakpoint test should create and delete breakpoints in `test.txt`; a plugin helper is appropriate only for inspecting or changing state through an API such as `BP_REF` that has no script equivalent.

## Running

Run the full suite:

```powershell
py src/tests/run.py --arch x64
```

Run a single test:

```powershell
py src/tests/run.py --arch x64 issue3808
```

Run a single variant:

```powershell
py src/tests/run.py --arch x64 membp/write
```

Run with a specific debug engine:

```powershell
py src/tests/run.py --arch x64 --engine GleeBug membp/range-write
```

List discovered tests:

```powershell
py src/tests/run.py --arch x64 --list
```

By default the runner suppresses debuggee console windows with `[Engine].NoConsoleWindow=1`.
Use `--console-window` to allow them again.

The runner prints each test result as soon as that test finishes, then prints the summary at the end. On failures it dumps only the filtered `[x64dbg-test]` lines from the test log to the console.

By default temporary artifacts are created under:

```text
%TEMP%/x64dbg-tests-<arch>-<engine>/<randomid>/
```

This keeps all temporary test runs under one parent directory for easier cleanup.

## CI and runner behavior

CI runs the active suite for both architectures and both debugger engines:

- x64 via `py src/tests/run.py --arch x64 --engine TitanEngine` and `--engine GleeBug`
- x86 via `py src/tests/run.py --arch x86 --engine TitanEngine` and `--engine GleeBug` (`x86` is accepted as an alias for `x32`)

The runner launches one `headless.exe -testing` process per test with:

- isolated `-userdir`
- a per-test `headless.ini` that selects the requested `[Engine].DebugEngine`
- `[Engine].NoConsoleWindow=1` by default to avoid console popup spam during automated tests
- explicit `-plugin` preloads from the test runtime directory
- `RedirectLog` to a per-test log file
- `-cf` pointing at the built `test.txt`

The canonical pass/fail signal is the final log line emitted by `testfinalize`:

```text
[x64dbg-test] FINAL status=pass asserts=3
[x64dbg-test] FINAL status=fail asserts=0 reason=no_asserts
```
