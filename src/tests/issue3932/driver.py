from __future__ import annotations

import argparse
import os
import queue
import subprocess
import sys
import threading
from pathlib import Path


PAYLOAD = "issue3932-payload"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Drive the issue #3932 attach-args regression test.")
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


def process_exit_reason(prefix: str, returncode: int) -> str:
    if os.name == "nt" and (returncode < 0 or returncode > 0xFF):
        return f"{prefix}_0x{returncode & 0xFFFFFFFF:08X}"
    return f"{prefix}_{returncode}"


def fail(log_path: Path, reason: str, message: str) -> int:
    append_log(log_path, f'[x64dbg-test] ASSERT FAIL source=driver message="{message}"')
    append_log(log_path, f"[x64dbg-test] FINAL status=fail asserts=1 reason={reason}")
    return 1


def wait_for_ready(process: subprocess.Popen[str], timeout: float) -> str | None:
    if process.stdout is None:
        return None

    lines: queue.Queue[str] = queue.Queue(maxsize=1)

    def read_line() -> None:
        lines.put(process.stdout.readline())

    threading.Thread(target=read_line, daemon=True).start()
    try:
        line = lines.get(timeout=timeout)
    except queue.Empty:
        return None
    if process.poll() is not None or not line:
        return None
    return line.strip()


def stop_target(process: subprocess.Popen[str]) -> None:
    if process.poll() is not None:
        return
    try:
        if process.stdin is not None:
            process.stdin.write("\n")
            process.stdin.flush()
        process.wait(timeout=5)
        return
    except Exception:
        pass
    try:
        process.terminate()
        process.wait(timeout=5)
    except Exception:
        try:
            process.kill()
        except Exception:
            pass


def launch_image(debuggee: Path) -> str:
    # QueryFullProcessImageName typically returns on-disk casing, so launching
    # through a swapped-case path is enough to trip the old case-sensitive strstr.
    swapped = debuggee.with_name(debuggee.name.swapcase())
    return str(swapped)


def main() -> int:
    args = parse_args()
    headless = Path(args.headless).resolve()
    debuggee = Path(args.debuggee).resolve()
    script = Path(args.script).resolve()
    runtime_dir = Path(args.runtime_dir).resolve()
    userdir = Path(args.userdir).resolve()
    log_path = Path(args.log).resolve()
    artifacts_dir = Path(args.artifacts_dir).resolve()
    headless_dir = headless.parent
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0) if args.no_console_window else 0
    image = launch_image(debuggee)
    target = subprocess.Popen(
        [image, PAYLOAD],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        creationflags=creationflags,
    )

    try:
        ready_line = wait_for_ready(target, timeout=10)
        if not ready_line or not ready_line.startswith("ready "):
            return fail(log_path, "target_not_ready", f"target did not print a ready line: {ready_line!r}")

        command = [
            str(headless),
            "-testing",
            "-userdir",
            str(userdir),
            "-p",
            str(target.pid),
        ]
        for plugin in sorted(runtime_dir.glob("*.dp32")) + list(sorted(runtime_dir.glob("*.dp64"))):
            command.extend(["-plugin", path_arg(plugin, headless_dir)])
        command.extend(
            [
                "-c",
                f'RedirectLog "{path_arg(log_path, headless_dir)}"',
                "-cf",
                path_arg(script, headless_dir),
            ]
        )
        print("[issue3932-driver] " + " ".join(command), flush=True)

        try:
            completed = subprocess.run(
                command,
                cwd=headless_dir,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=args.timeout,
                text=True,
                encoding="utf-8",
                errors="replace",
                creationflags=creationflags,
            )
        except subprocess.TimeoutExpired as exc:
            output = exc.stdout.decode("utf-8", errors="replace") if isinstance(exc.stdout, bytes) else (exc.stdout or "")
            (artifacts_dir / "headless.stdout.txt").write_text(output + "\n[TIMEOUT]\n", encoding="utf-8", errors="replace")
            return fail(log_path, "headless_timeout", "headless timed out while attaching with -p")

        (artifacts_dir / "headless.stdout.txt").write_text(completed.stdout, encoding="utf-8", errors="replace")
        if completed.returncode != 0:
            reason = process_exit_reason("headless_exit", completed.returncode)
            return fail(log_path, reason, f"headless exited with {completed.returncode} ({reason})")
        return 0
    finally:
        stop_target(target)


if __name__ == "__main__":
    raise SystemExit(main())
