from __future__ import annotations

import argparse
import os
import re
import subprocess
import threading
import time
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate and open a deterministic minidump replay fixture.")
    parser.add_argument("--headless", required=True)
    parser.add_argument("--debuggee", required=True)
    parser.add_argument("--script", required=True)
    parser.add_argument("--runtime-dir", required=True)
    parser.add_argument("--userdir", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--artifacts-dir", required=True)
    parser.add_argument("--engine", required=True)
    parser.add_argument("--timeout", type=int, required=True)
    parser.add_argument("--no-console-window", action="store_true")
    return parser.parse_args()


def append_log(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8", errors="replace") as stream:
        stream.write(text + ("" if text.endswith("\n") else "\n"))


class Headless:
    def __init__(self, executable: Path, userdir: Path, creationflags: int) -> None:
        self.process = subprocess.Popen(
            [str(executable), "-userdir", str(userdir)],
            cwd=executable.parent,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            creationflags=creationflags,
            bufsize=1,
        )
        self.lines: list[str] = []
        self.condition = threading.Condition()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self) -> None:
        assert self.process.stdout is not None
        for line in self.process.stdout:
            with self.condition:
                self.lines.append(line.rstrip())
                self.condition.notify_all()

    def send(self, command: str) -> None:
        assert self.process.stdin is not None
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def wait_count(self, text: str, count: int, timeout: float) -> bool:
        deadline = time.monotonic() + timeout
        with self.condition:
            while sum(text in line for line in self.lines) < count:
                if self.process.poll() is not None:
                    return False
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return False
                self.condition.wait(min(remaining, 0.25))
        return True

    def matching(self, text: str) -> list[str]:
        with self.condition:
            return [line for line in self.lines if text in line]

    def output(self) -> str:
        with self.condition:
            return "\n".join(self.lines) + "\n"


def main() -> int:
    args = parse_args()
    headless_path = Path(args.headless).resolve()
    target_path = Path(args.debuggee).resolve()
    runtime_dir = Path(args.runtime_dir).resolve()
    userdir = Path(args.userdir).resolve()
    artifacts_dir = Path(args.artifacts_dir).resolve()
    log_path = Path(args.log).resolve()
    runtime_dir.mkdir(parents=True, exist_ok=True)
    artifacts_dir.mkdir(parents=True, exist_ok=True)
    if args.engine != "DbgEng":
        append_log(log_path, f'[x64dbg-test] FINAL status=skip asserts=0 reason=unsupported_engine_{args.engine}')
        return 0

    dump_path = runtime_dir / "fixture.dmp"
    dump_path.unlink(missing_ok=True)

    creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0) if args.no_console_window else 0
    generated = subprocess.run(
        [str(target_path), str(dump_path)],
        cwd=runtime_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=min(args.timeout, 60),
        text=True,
        encoding="utf-8",
        errors="replace",
        creationflags=creationflags,
    )
    if generated.returncode != 0 or not dump_path.is_file():
        append_log(log_path, '[x64dbg-test] ASSERT FAIL source=driver message="fixture generation failed"')
        append_log(log_path, "[x64dbg-test] FINAL status=fail asserts=1 reason=fixture_generation")
        return 1

    debugger = Headless(headless_path, userdir, creationflags)

    def finish_failure(reason: str, message: str) -> int:
        try:
            if debugger.process.poll() is None:
                debugger.send("stop")
                debugger.send("exit")
                debugger.process.wait(timeout=15)
        except Exception:
            debugger.process.kill()
            debugger.process.wait(timeout=5)
        (artifacts_dir / "headless-output.txt").write_text(debugger.output(), encoding="utf-8", errors="replace")
        append_log(log_path, f'[x64dbg-test] ASSERT FAIL source=driver message="{message}"')
        append_log(log_path, f"[x64dbg-test] FINAL status=fail asserts=1 reason={reason}")
        return 1

    timeout = max(30, args.timeout)
    if not debugger.wait_count("[headless] entering command loop", 1, timeout):
        return finish_failure("headless_start", "headless command loop did not start")

    # Keep the command argument relative and whitespace-free because the
    # headless command-language dispatcher performs its own quoting pass.
    artifact_arg = os.path.relpath(dump_path, headless_path.parent).replace("\\", "/")

    expected_cip: str | None = None
    assertion_messages: list[str] = []
    for iteration in (1, 2):
        debugger.send(f"initreplay {artifact_arg}")
        if not debugger.wait_count("System breakpoint reached!", iteration, timeout):
            return finish_failure("replay_open", f"replay session {iteration} did not reach its initial pause")
        if not debugger.wait_count("Stored dump exception: C0000005", iteration, 10):
            return finish_failure("stored_exception", "stored access violation was not exposed")
        assertion_messages.append(f"session {iteration} exposed the stored exception")
        thread_lines = [line for line in debugger.matching("Thread ") if " created, Entry:" in line]
        if len(thread_lines) < iteration * 2:
            return finish_failure("thread_enumeration", f"session {iteration} did not enumerate captured worker threads")
        assertion_messages.append(f"session {iteration} enumerated captured worker threads")

        debugger.send(f"log REPLAY{iteration}_CIP={{p:cip}}")
        debugger.send(f"log REPLAY{iteration}_BYTE={{byte:[cip]}}")
        debugger.send(f"log REPLAY{iteration}_PEB={{p:peb()}}")
        if not debugger.wait_count(f"REPLAY{iteration}_PEB=", 1, 10):
            return finish_failure("state_query", f"session {iteration} state queries did not complete")

        cip_lines = debugger.matching(f"REPLAY{iteration}_CIP=")
        byte_lines = debugger.matching(f"REPLAY{iteration}_BYTE=")
        peb_lines = debugger.matching(f"REPLAY{iteration}_PEB=")
        cip_match = re.search(r"=([0-9A-Fa-f]+)$", cip_lines[-1]) if cip_lines else None
        byte_match = re.search(r"=([0-9A-Fa-f]+)$", byte_lines[-1]) if byte_lines else None
        peb_match = re.search(r"=([0-9A-Fa-f]+)$", peb_lines[-1]) if peb_lines else None
        if not cip_match or int(cip_match.group(1), 16) == 0:
            return finish_failure("context", "captured instruction pointer is unavailable")
        if not byte_match:
            return finish_failure("memory", "captured instruction bytes are unavailable")
        if not peb_match or int(peb_match.group(1), 16) == 0:
            return finish_failure("peb", "captured PEB is unavailable")
        if expected_cip is None:
            expected_cip = cip_match.group(1).lower()
        elif cip_match.group(1).lower() != expected_cip:
            return finish_failure("repeat_state", "reopened dump produced a different instruction pointer")
        assertion_messages.extend([
            f"session {iteration} returned a captured context",
            f"session {iteration} returned captured instruction memory",
            f"session {iteration} returned the captured PEB",
        ])

        if iteration == 1:
            debugger.send("run")
            if not debugger.wait_count("This replay artifact does not support execution.", 1, 10):
                return finish_failure("immutable_execution", "minidump run was not rejected")
            assertion_messages.append("minidump execution was rejected")

        debugger.send("stop")
        if not debugger.wait_count("Debugging stopped!", iteration, 30):
            return finish_failure("replay_close", f"replay session {iteration} did not close")

    debugger.send("exit")
    try:
        debugger.process.wait(timeout=30)
    except subprocess.TimeoutExpired:
        return finish_failure("headless_exit", "headless did not exit")
    (artifacts_dir / "headless-output.txt").write_text(debugger.output(), encoding="utf-8", errors="replace")
    if debugger.process.returncode != 0:
        return finish_failure("process_exit", f"headless exited with {debugger.process.returncode}")

    for message in assertion_messages:
        append_log(log_path, f'[x64dbg-test] ASSERT PASS source=driver message="{message}"')
    append_log(log_path, f"[x64dbg-test] FINAL status=pass asserts={len(assertion_messages)}")
    dump_path.unlink(missing_ok=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
