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


@dataclass
class TestCase:
    rel: str
    source_dir: Path
    runtime_dir: Path
    runtime_script: Path
    debuggee: Path
    plugins: list[Path]
    fallback_check: Path | None


@dataclass
class TestResult:
    rel: str
    passed: bool
    reason: str
    asserts: int | None
    returncode: int | None
    artifact_dir: Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run x64dbg convention-based headless tests.")
    parser.add_argument("tests", nargs="*", help="Optional test ids relative to src/tests, for example: issue3808 or membp/write")
    parser.add_argument("--arch", choices=["x64", "x32", "x86"], default="x64", help="Architecture to run. Default: x64. x86 is accepted as an alias for x32")
    parser.add_argument("--headless", help="Path to headless.exe. Defaults to bin/<arch>/headless.exe")
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


def variant_check_path(source_dir: Path, variant: str) -> Path | None:
    if variant:
        check_path = source_dir / f"check.{variant}.py"
        if check_path.is_file():
            return check_path
    check_path = source_dir / "check.py"
    return check_path if check_path.is_file() else None


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
                fallback_check=variant_check_path(script.parent, variant),
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


def run_test(headless: Path, test: TestCase, timeout: int, artifact_root: Path) -> TestResult:
    artifact_dir = artifact_root / test.rel.replace("/", "__")
    if artifact_dir.exists():
        shutil.rmtree(artifact_dir, ignore_errors=True)
    artifact_dir.mkdir(parents=True, exist_ok=True)

    userdir = artifact_dir / "userdir"
    userdir.mkdir(parents=True, exist_ok=True)
    log_path = artifact_dir / "debug.log"
    stdout_path = artifact_dir / "stdout.txt"

    headless_dir = headless.parent
    command = [
        str(headless),
        "-testing",
        "-userdir",
        str(userdir),
    ]
    for plugin in test.plugins:
        command.extend(["-plugin", path_arg(plugin, headless_dir)])
    command.extend(
        [
            "-c",
            f'RedirectLog "{path_arg(log_path, headless_dir)}"',
            "-cf",
            path_arg(test.runtime_script, headless_dir),
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
        stdout_path.write_text((exc.stdout or "") + "\n[TIMEOUT]\n", encoding="utf-8", errors="replace")
        return TestResult(test.rel, False, "timeout", None, None, artifact_dir)

    passed, asserts, reason = parse_final_line(log_path)
    if completed.returncode != 0:
        passed = False
        if reason == "pass":
            reason = f"process_exit_{completed.returncode}"
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
        artifact_root.mkdir(parents=True, exist_ok=True)
    else:
        artifact_parent = Path(tempfile.gettempdir()) / f"x64dbg-tests-{args.arch}"
        artifact_parent.mkdir(parents=True, exist_ok=True)
        artifact_root = artifact_parent / secrets.token_hex(8)
        artifact_root.mkdir(parents=True, exist_ok=False)
        temporary_root = True

    results: list[TestResult] = []
    overall_success = True
    for test in tests:
        result = run_test(headless, test, args.timeout, artifact_root)
        results.append(result)
        print_test_result(result)
        print_failure_logs(result)
        overall_success = overall_success and result.passed

    keep_artifacts = args.keep_artifacts or not overall_success or not temporary_root
    print_results(results, artifact_root if keep_artifacts else None)

    if temporary_root and not keep_artifacts and artifact_root.exists():
        shutil.rmtree(artifact_root, ignore_errors=True)

    return 0 if overall_success else 1


if __name__ == "__main__":
    raise SystemExit(main())
