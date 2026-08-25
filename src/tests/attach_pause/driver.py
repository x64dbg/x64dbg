from __future__ import annotations

import argparse
import ctypes
import os
import queue
import subprocess
import sys
import threading
import time
from pathlib import Path

WM_QUIT = 0x0012
WM_APP = 0x8000


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Drive the attach + pause regression test.")
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


def path_arg(path: Path, cwd: Path) -> str:
    try:
        return os.path.relpath(path, cwd)
    except ValueError:
        return str(path)


def append_log(log_path: Path, text: str) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("a", encoding="utf-8", errors="replace") as log_file:
        log_file.write(text)
        if not text.endswith("\n"):
            log_file.write("\n")


def wait_for_ready(process: subprocess.Popen[str], timeout: float) -> str | None:
    assert process.stdout is not None
    lines: queue.Queue[str] = queue.Queue(maxsize=1)
    threading.Thread(target=lambda: lines.put(process.stdout.readline()), daemon=True).start()
    try:
        line = lines.get(timeout=timeout)
    except queue.Empty:
        return None
    return line.strip() if line else None


class Headless:
    def __init__(self, command: list[str], cwd: Path, creationflags: int) -> None:
        self.process = subprocess.Popen(
            command,
            cwd=cwd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            creationflags=creationflags,
        )
        self.lines: list[str] = []
        self.lock = threading.Lock()
        threading.Thread(target=self._reader, daemon=True).start()

    def _reader(self) -> None:
        assert self.process.stdout is not None
        for line in self.process.stdout:
            with self.lock:
                self.lines.append(line.rstrip())

    def send(self, command: str) -> None:
        assert self.process.stdin is not None
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()

    def wait_for_line(self, predicate, timeout: float) -> str | None:
        deadline = time.monotonic() + timeout
        seen = 0
        while time.monotonic() < deadline:
            with self.lock:
                new_lines = self.lines[seen:]
                seen = len(self.lines)
            for line in new_lines:
                if predicate(line):
                    return line
            if self.process.poll() is not None:
                return None
            time.sleep(0.1)
        return None

    def wait_for_quiescence(self, idle: float, timeout: float) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            with self.lock:
                count = len(self.lines)
            time.sleep(idle)
            with self.lock:
                if len(self.lines) == count:
                    return

    def dump(self, path: Path) -> None:
        with self.lock:
            path.write_text("\n".join(self.lines) + "\n", encoding="utf-8", errors="replace")


def main() -> int:
    args = parse_args()
    headless_path = Path(args.headless).resolve()
    debuggee = Path(args.debuggee).resolve()
    userdir = Path(args.userdir).resolve()
    log_path = Path(args.log).resolve()
    artifacts_dir = Path(args.artifacts_dir).resolve()
    headless_dir = headless_path.parent
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    # The log is written after headless shuts down, because RedirectLog keeps
    # the file locked while headless is running.
    def fail(reason: str, message: str) -> int:
        shutdown_headless()
        append_log(log_path, f'[x64dbg-test] ASSERT FAIL source=driver message="{message}"')
        append_log(log_path, f"[x64dbg-test] FINAL status=fail asserts=1 reason={reason}")
        return 1

    def shutdown_headless() -> None:
        if headless is None or headless.process.poll() is not None:
            return
        try:
            headless.send("detach")
            headless.wait_for_line(lambda line: line == "[STATE] stopped", 10)
            headless.send("exit")
            headless.process.wait(timeout=10)
        except Exception:
            headless.process.kill()
            try:
                headless.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                pass

    # test.txt drives the base test, test.breakin.txt the break-in fallback.
    script_name = Path(args.script).name
    breakin_mode = script_name == "test.breakin.txt"

    creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0) if args.no_console_window else 0
    target = subprocess.Popen(
        [str(debuggee)] + (["block"] if breakin_mode else []),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        creationflags=creationflags,
    )

    headless: Headless | None = None
    main_tid = 0
    try:
        ready_line = wait_for_ready(target, timeout=10)
        if not ready_line or not ready_line.startswith("ready "):
            return fail("target_not_ready", f"target did not print a ready line: {ready_line!r}")
        pid, main_tid = (int(part) for part in ready_line.split()[1:3])

        command = [
            str(headless_path),
            "-userdir",
            str(userdir),
            "-c",
            f'RedirectLog "{path_arg(log_path, headless_dir)}"',
        ]
        print("[attach-pause-driver] " + " ".join(command), flush=True)
        headless = Headless(command, headless_dir, creationflags)

        # Attach through the interactive command handler (DbgCmdExecAsync), which is
        # the same code path as the GUI attach dialog. This does not pause.
        headless.send(f"attach .{pid}")
        if headless.wait_for_line(lambda line: "Attached to process!" in line, 30) is None:
            return fail("attach_failed", "attach command did not report success")

        # Let the attach event storm and symbol loading settle.
        headless.wait_for_quiescence(idle=2, timeout=30)

        if breakin_mode:
            # Every thread of the target blocks forever, so the first pause
            # request cannot interrupt anything.
            headless.send("pause")
            if headless.wait_for_line(lambda line: line == "[STATE] paused", 4) is not None:
                return fail("unexpected_pause", "first pause request should not break a fully blocked debuggee")

            # A repeated pause request after a few seconds must fall back to a
            # break-in thread and interrupt the debuggee.
            headless.send("pause")
            if headless.wait_for_line(lambda line: line == "[STATE] paused", 10) is None:
                return fail("breakin_no_effect", "repeated pause request did not break in")

            shutdown_headless()
            append_log(log_path, '[x64dbg-test] ASSERT PASS source=driver message="first pause request did not pause the blocked debuggee"')
            append_log(log_path, '[x64dbg-test] ASSERT PASS source=driver message="repeated pause request paused via break-in thread"')
            append_log(log_path, "[x64dbg-test] FINAL status=pass asserts=2")
            return 0

        # Make a worker thread produce a debug event (OutputDebugString) and
        # then block forever, so the debugger's active thread is a thread that
        # never executes again.
        ctypes.windll.user32.PostThreadMessageW(main_tid, WM_APP, 0, 0)
        if headless.wait_for_line(lambda line: "attach_pause: parking" in line, 10) is None:
            return fail("no_park_marker", "target did not report the parked worker thread")

        # The pause command must interrupt the debuggee even though the active
        # thread is a parked worker thread that never executes.
        headless.send("pause")
        if headless.wait_for_line(lambda line: line == "[STATE] paused", 10) is None:
            return fail("pause_no_effect", "debugger did not pause after the pause command")

        shutdown_headless()
        append_log(log_path, '[x64dbg-test] ASSERT PASS source=driver message="pause interrupted the attached debuggee"')
        append_log(log_path, "[x64dbg-test] FINAL status=pass asserts=1")
        return 0
    finally:
        if headless is not None:
            headless.dump(artifacts_dir / "headless.stdout.txt")
            if headless.process.poll() is None:
                headless.process.kill()
        if target.poll() is None:
            if main_tid:
                ctypes.windll.user32.PostThreadMessageW(main_tid, WM_QUIT, 0, 0)
            try:
                target.wait(timeout=5)
            except subprocess.TimeoutExpired:
                target.kill()


if __name__ == "__main__":
    raise SystemExit(main())
