from __future__ import annotations

import argparse
import os
import re
import secrets
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

FINAL_RE = re.compile(r"^\[x64dbg-test\] FINAL status=(?P<status>pass|fail) asserts=(?P<asserts>\d+)(?: reason=(?P<reason>\S+))?$")
TEST_LOG_RE = re.compile(r"^\[x64dbg-test\]")
ARTIFACT_GITIGNORE = ".gitignore"
ARTIFACT_GITIGNORE_MARKER = "# x64dbg-test\n*"
ABSOLUTE_CF_MARKER = "absolute_cf"
DEBUG_ENGINE_VALUES = {
    "TitanEngine": 0,
    "GleeBug": 1,
    "StaticEngine": 2,
}
DEBUG_ENGINE_ALIASES = {name.lower(): name for name in DEBUG_ENGINE_VALUES}


@dataclass
class TestCase:
    rel: str
    source_dir: Path
    runtime_dir: Path
    runtime_script: Path
    debuggee: Path
    plugins: list[Path]
    driver: Path | None
    fallback_check: Path | None
    absolute_command_file: bool


@dataclass
class TestResult:
    rel: str
    passed: bool
    reason: str
    asserts: int | None
    returncode: int | None
    artifact_dir: Path


def normalize_engine(engine: str) -> str:
    canonical = DEBUG_ENGINE_ALIASES.get(engine.strip().lower())
    if canonical is None:
        choices = ", ".join(sorted(DEBUG_ENGINE_VALUES))
        raise argparse.ArgumentTypeError(f"invalid engine '{engine}', choose from: {choices}")
    return canonical


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run x64dbg convention-based headless tests.")
    parser.add_argument("tests", nargs="*", help="Optional test ids relative to src/tests, for example: issue3808 or membp/write")
    parser.add_argument("--arch", choices=["x64", "x32", "x86"], default="x64", help="Architecture to run. Default: x64. x86 is accepted as an alias for x32")
    parser.add_argument("--engine", type=normalize_engine, choices=sorted(DEBUG_ENGINE_VALUES), default="TitanEngine", help="Debug engine to use. Default: TitanEngine")
    parser.add_argument("--headless", help="Path to headless.exe. Defaults to bin/<arch>/headless.exe")
    parser.add_argument("--console-window", action="store_true", help="Allow debuggee console windows. Default: suppress console windows during tests")
    parser.add_argument("--timeout", type=int, default=90, help="Per-test timeout in seconds. Default: 90")
    parser.add_argument("--artifacts-dir", help="Directory to keep artifacts in. Defaults to a temporary directory")
    parser.add_argument("--keep-artifacts", action="store_true", help="Keep artifacts even if all tests pass")
    parser.add_argument("--list", action="store_true", help="List discovered tests and exit")
    return parser.parse_args()


def ensure_file(path: Path, description: str) -> Path:
    if not path.is_file():
        raise FileNotFoundError(f"Missing {description}: {path}")
    return path.resolve()


def normalize_arch(arch: str) -> str:
    return "x32" if arch == "x86" else arch


def path_arg(path: Path, cwd: Path) -> str:
    try:
        return os.path.relpath(path, cwd)
    except ValueError:
        return str(path)


def timeout_output(output: str | bytes | None) -> str:
    if isinstance(output, bytes):
        return output.decode("utf-8", errors="replace")
    return output or ""


def process_exit_reason(prefix: str, returncode: int) -> str:
    if os.name == "nt" and (returncode < 0 or returncode > 0xFF):
        return f"{prefix}_0x{returncode & 0xFFFFFFFF:08X}"
    return f"{prefix}_{returncode}"


def parse_test_variant(script_name: str) -> str | None:
    if not script_name.startswith("test") or not script_name.endswith(".txt"):
        return None

    variant = script_name[4:-4]
    if variant.startswith("."):
        variant = variant[1:]

    if variant == "disabled" or variant.startswith("disabled."):
        return None

    return variant


def test_id_from_rel(rel_dir: str, variant: str) -> str:
    if not variant:
        return rel_dir
    return f"{rel_dir}/{variant}" if rel_dir else variant


def variant_python_hook_path(source_dir: Path, variant: str, stem: str) -> Path | None:
    if variant:
        hook_path = source_dir / f"{stem}.{variant}.py"
        if hook_path.is_file():
            return hook_path
    hook_path = source_dir / f"{stem}.py"
    return hook_path if hook_path.is_file() else None


def variant_check_path(source_dir: Path, variant: str) -> Path | None:
    return variant_python_hook_path(source_dir, variant, "check")


def variant_driver_path(source_dir: Path, variant: str) -> Path | None:
    return variant_python_hook_path(source_dir, variant, "driver")


def discover_tests(repo_root: Path, arch: str, requested: set[str], validate_runtime: bool = True) -> list[TestCase]:
    source_root = repo_root / "src" / "tests"
    runtime_root = repo_root / "bin" / arch / "tests"
    plugin_suffix = ".dp64" if arch == "x64" else ".dp32"

    tests: list[TestCase] = []
    seen_rel: set[str] = set()
    for script in sorted(source_root.rglob("test*.txt")):
        variant = parse_test_variant(script.name)
        if variant is None:
            continue

        rel_dir = script.parent.relative_to(source_root).as_posix()
        rel = test_id_from_rel(rel_dir, variant)
        if rel in seen_rel:
            raise FileExistsError(f"Duplicate test id discovered: {rel}")
        if requested and rel not in requested:
            continue

        runtime_dir = runtime_root / Path(rel_dir)
        runtime_script = runtime_dir / script.name
        debuggee = runtime_root / f"{rel_dir}.exe"
        tests.append(
            TestCase(
                rel=rel,
                source_dir=script.parent,
                runtime_dir=runtime_dir,
                runtime_script=runtime_script,
                debuggee=debuggee,
                plugins=sorted(runtime_dir.glob(f"*{plugin_suffix}")),
                driver=variant_driver_path(script.parent, variant),
                fallback_check=variant_check_path(script.parent, variant),
                absolute_command_file=(script.parent / ABSOLUTE_CF_MARKER).is_file(),
            )
        )
        seen_rel.add(rel)

    if requested:
        found = {test.rel for test in tests}
        missing = sorted(requested - found)
        if missing:
            raise FileNotFoundError(f"Unknown tests: {', '.join(missing)}")

    if not tests:
        raise FileNotFoundError("No tests were discovered under src/tests.")

    if validate_runtime:
        for test in tests:
            ensure_file(test.runtime_script, f"runtime script for {test.rel}")
            ensure_file(test.debuggee, f"debuggee for {test.rel}")

    return tests


def ensure_debug_engine_runtime(headless: Path, engine: str) -> None:
    engine_runtime = {
        "TitanEngine": headless.parent / "TitanEngine.dll",
        "GleeBug": headless.parent / "GleeBug" / "TitanEngine.dll",
        "StaticEngine": headless.parent / "StaticEngine" / "TitanEngine.dll",
    }[engine]
    ensure_file(engine_runtime, f"{engine} debug engine runtime")


def is_managed_dir(path: Path) -> bool:
    gitignore = path / ARTIFACT_GITIGNORE
    if not gitignore.is_file():
        return False
    contents = gitignore.read_text(encoding="utf-8", errors="replace").replace("\r\n", "\n").strip()
    return ARTIFACT_GITIGNORE_MARKER in contents


def ensure_managed_dir(path: Path) -> None:
    if path.exists():
        if not path.is_dir():
            raise RuntimeError(f"Artifact path is not a directory: {path}")
        if is_managed_dir(path):
            return
        if any(path.iterdir()):
            raise RuntimeError(f"Refusing to use non-empty unmanaged artifact directory: {path}")
    path.mkdir(parents=True, exist_ok=True)
    (path / ARTIFACT_GITIGNORE).write_text("# x64dbg-test\n*\n", encoding="utf-8")


def remove_managed_dir(path: Path) -> None:
    if not path.exists():
        return
    if not is_managed_dir(path):
        raise RuntimeError(f"Refusing to delete unmanaged artifact directory: {path}")
    shutil.rmtree(path, ignore_errors=True)


def write_headless_ini(userdir: Path, engine: str, no_console_window: bool) -> None:
    (userdir / "headless.ini").write_text(
        "[Engine]\n"
        f"DebugEngine={DEBUG_ENGINE_VALUES[engine]}\n"
        f"NoConsoleWindow={1 if no_console_window else 0}\n",
        encoding="utf-8",
    )


def parse_final_line(log_path: Path) -> tuple[bool, int | None, str]:
    if not log_path.is_file():
        return False, None, "missing_log"

    final_match: re.Match[str] | None = None
    for line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = FINAL_RE.match(line.strip())
        if match:
            final_match = match

    if final_match is None:
        return False, None, "missing_final"

    status = final_match.group("status")
    asserts = int(final_match.group("asserts"))
    reason = final_match.group("reason") or ("pass" if status == "pass" else "fail")
    return status == "pass", asserts, reason


def run_fallback_check(check_path: Path, log_path: Path, userdir: Path, runtime_dir: Path, artifact_dir: Path) -> tuple[bool, str]:
    command = [
        sys.executable,
        str(check_path),
        "--log",
        str(log_path),
        "--userdir",
        str(userdir),
        "--runtime-dir",
        str(runtime_dir),
        "--artifacts-dir",
        str(artifact_dir),
    ]
    completed = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace")
    (artifact_dir / "check.stdout.txt").write_text(completed.stdout, encoding="utf-8", errors="replace")
    (artifact_dir / "check.stderr.txt").write_text(completed.stderr, encoding="utf-8", errors="replace")
    if completed.returncode != 0:
        return False, f"fallback_check_failed({completed.returncode})"
    return True, "pass"


def run_driver_test(headless: Path, test: TestCase, timeout: int, artifact_dir: Path, userdir: Path, log_path: Path, stdout_path: Path, engine: str, no_console_window: bool) -> TestResult:
    assert test.driver is not None
    command = [
        sys.executable,
        str(test.driver),
        "--headless",
        str(headless),
        "--debuggee",
        str(test.debuggee),
        "--script",
        str(test.runtime_script),
        "--runtime-dir",
        str(test.runtime_dir),
        "--userdir",
        str(userdir),
        "--log",
        str(log_path),
        "--artifacts-dir",
        str(artifact_dir),
        "--engine",
        engine,
        "--timeout",
        str(timeout),
    ]
    if no_console_window:
        command.append("--no-console-window")

    try:
        creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        completed = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout + 15,
            text=True,
            encoding="utf-8",
            errors="replace",
            creationflags=creationflags,
        )
        stdout_path.write_text(completed.stdout, encoding="utf-8", errors="replace")
    except subprocess.TimeoutExpired as exc:
        stdout_path.write_text(timeout_output(exc.stdout) + "\n[DRIVER TIMEOUT]\n", encoding="utf-8", errors="replace")
        return TestResult(test.rel, False, "driver_timeout", None, None, artifact_dir)

    passed, asserts, reason = parse_final_line(log_path)
    if completed.returncode != 0:
        passed = False
        if reason in {"pass", "missing_final"}:
            reason = process_exit_reason("driver_exit", completed.returncode)
    if passed and test.fallback_check is not None:
        passed, reason = run_fallback_check(test.fallback_check, log_path, userdir, test.runtime_dir, artifact_dir)

    return TestResult(test.rel, passed, reason, asserts, completed.returncode, artifact_dir)


def run_test(headless: Path, test: TestCase, timeout: int, artifact_root: Path, engine: str, no_console_window: bool) -> TestResult:
    artifact_dir = artifact_root / test.rel.replace("/", "__")
    if artifact_dir.exists():
        remove_managed_dir(artifact_dir)
    ensure_managed_dir(artifact_dir)

    userdir = artifact_dir / "userdir"
    userdir.mkdir(parents=True, exist_ok=True)
    write_headless_ini(userdir, engine, no_console_window)
    log_path = artifact_dir / "debug.log"
    stdout_path = artifact_dir / "stdout.txt"

    if test.driver is not None:
        return run_driver_test(headless, test, timeout, artifact_dir, userdir, log_path, stdout_path, engine, no_console_window)

    headless_dir = headless.parent
    command = [
        str(headless),
        "-testing",
        "-userdir",
        str(userdir),
    ]
    for plugin in test.plugins:
        command.extend(["-plugin", path_arg(plugin, headless_dir)])
    command_file = str(test.runtime_script) if test.absolute_command_file else path_arg(test.runtime_script, headless_dir)
    command.extend(
        [
            "-c",
            f'RedirectLog "{path_arg(log_path, headless_dir)}"',
            "-cf",
            command_file,
        ]
    )

    try:
        creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        completed = subprocess.run(
            command,
            cwd=headless_dir,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout,
            text=True,
            encoding="utf-8",
            errors="replace",
            creationflags=creationflags,
        )
        stdout_path.write_text(completed.stdout, encoding="utf-8", errors="replace")
    except subprocess.TimeoutExpired as exc:
        stdout_path.write_text(timeout_output(exc.stdout) + "\n[TIMEOUT]\n", encoding="utf-8", errors="replace")
        return TestResult(test.rel, False, "timeout", None, None, artifact_dir)

    passed, asserts, reason = parse_final_line(log_path)
    if completed.returncode != 0:
        passed = False
        if reason == "pass":
            reason = process_exit_reason("process_exit", completed.returncode)
    if passed and test.fallback_check is not None:
        passed, reason = run_fallback_check(test.fallback_check, log_path, userdir, test.runtime_dir, artifact_dir)

    return TestResult(test.rel, passed, reason, asserts, completed.returncode, artifact_dir)


def print_test_result(result: TestResult) -> None:
    asserts = "?" if result.asserts is None else str(result.asserts)
    status = "PASS" if result.passed else "FAIL"
    print(f"{status:4} {result.rel}  asserts={asserts}  reason={result.reason}", flush=True)


def print_failure_logs(result: TestResult) -> None:
    if result.passed:
        return

    log_path = result.artifact_dir / "debug.log"
    if not log_path.is_file():
        return

    lines = log_path.read_text(encoding="utf-8", errors="replace").splitlines()
    test_lines = [line for line in lines if TEST_LOG_RE.match(line)]
    if not test_lines:
        return

    print(f"--- {result.rel} test log ---", flush=True)
    for line in test_lines:
        print(line, flush=True)
    print(f"--- end {result.rel} test log ---", flush=True)


def print_results(results: Iterable[TestResult], artifact_root: Path | None) -> None:
    results = list(results)
    passed = sum(1 for result in results if result.passed)
    total = len(results)
    print(f"Summary: {passed}/{total} passed")
    if artifact_root is not None:
        print(f"Artifacts: {artifact_root}")


def main() -> int:
    if os.name != "nt":
        print("This runner is Windows-only.", file=sys.stderr)
        return 2

    args = parse_args()
    args.arch = normalize_arch(args.arch)
    repo_root = Path(__file__).resolve().parents[2]
    headless = ensure_file(Path(args.headless) if args.headless else repo_root / "bin" / args.arch / "headless.exe", "headless executable")
    ensure_debug_engine_runtime(headless, args.engine)

    requested = {name.replace("\\", "/") for name in args.tests}
    tests = discover_tests(repo_root, args.arch, requested, validate_runtime=not args.list)

    if args.list:
        for test in tests:
            print(test.rel)
        return 0

    artifact_root: Path
    temporary_root = False
    if args.artifacts_dir:
        artifact_root = Path(args.artifacts_dir).resolve()
        ensure_managed_dir(artifact_root)
    else:
        artifact_parent = Path(tempfile.gettempdir()) / f"x64dbg-tests-{args.arch}-{args.engine}"
        artifact_parent.mkdir(parents=True, exist_ok=True)
        artifact_root = artifact_parent / secrets.token_hex(8)
        ensure_managed_dir(artifact_root)
        temporary_root = True

    # Create the WER destination only after claiming the artifact directory;
    # pre-populating it in CI would trip the unmanaged-directory safety check.
    (artifact_root / "crash-dumps").mkdir(exist_ok=True)

    results: list[TestResult] = []
    overall_success = True
    for test in tests:
        result = run_test(headless, test, args.timeout, artifact_root, args.engine, not args.console_window)
        results.append(result)
        print_test_result(result)
        print_failure_logs(result)
        overall_success = overall_success and result.passed

    keep_artifacts = args.keep_artifacts or not overall_success or not temporary_root
    print_results(results, artifact_root if keep_artifacts else None)

    if temporary_root and not keep_artifacts and artifact_root.exists():
        remove_managed_dir(artifact_root)

    return 0 if overall_success else 1


if __name__ == "__main__":
    raise SystemExit(main())
