# x64dbg tests

This is the convention-based automated test tree.

## Source layout

Each test lives under:

```text
src/tests/<rel>/
  test.txt
  optional test.<variant>.txt
  optional check.py
  optional check.<variant>.py
  optional *.cpp / plugin sources
```

`test.txt` keeps the historical test id `<rel>`.

Additional scripts in the same directory are exposed as `<rel>/<variant>`. For example:

- `src/tests/membp/test.txt` -> `membp`
- `src/tests/membp/test.write.txt` -> `membp/write`

## Runtime layout

Build output is placed under:

```text
bin/<arch>/tests/
  <rel>.exe
  <rel>/
    test.txt
    test.<variant>.txt
    *.dp32 / *.dp64
```

## Running

Example:

```powershell
py src/tests/run.py --arch x64
```

Run a single test:

```powershell
py src/tests/run.py --arch x64 issue3808
```

Run a single variant from a shared directory:

```powershell
py src/tests/run.py --arch x64 membp/write
```

List discovered tests:

```powershell
py src/tests/run.py --arch x64 --list
```

The runner prints each test result as soon as that test finishes, then prints the summary at the end. On failures it dumps only the filtered `[x64dbg-test]` lines from the test log to the console.

By default temporary artifacts are created under:

```text
%TEMP%/x64dbg-tests-<arch>/<randomid>/
```

This keeps all temporary test runs under one parent directory for easier cleanup.

CI also runs the active suite in GitHub Actions for both architectures:

- x64 via `py src/tests/run.py --arch x64`
- x86 via `py src/tests/run.py --arch x86` (`x86` is accepted as an alias for `x32`)

The runner launches one `headless.exe -testing` process per test with:

- isolated `-userdir`
- explicit `-plugin` preloads from the test runtime directory
- `RedirectLog` to a per-test log file
- `-cf` pointing at the built `test.txt`

The canonical pass/fail signal is the final log line emitted by `testfinalize`:

```text
[x64dbg-test] FINAL status=pass asserts=3
[x64dbg-test] FINAL status=fail asserts=0 reason=no_asserts
```
