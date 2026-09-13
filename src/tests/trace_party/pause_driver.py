"""External user of headless stdin for Pause tests (no debugger internals).

A startup script awaits a trace on the command thread, so it cannot issue its
own concurrent Pause. These scripts instead use normal interactive commands;
WAIT comments only synchronize the driver with observable states/target data.
"""
from __future__ import annotations

import queue
import subprocess
import threading
import time
from pathlib import Path


def run(args, script: str) -> int:
    headless = Path(args.headless).resolve()
    artifacts = Path(args.artifacts_dir).resolve()
    log = Path(args.log).resolve()
    command = [str(headless), "-userdir", str(Path(args.userdir).resolve()),
               "-c", f'RedirectLog "{log}"']
    process = subprocess.Popen(command, cwd=headless.parent, stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, encoding="utf-8", errors="replace",
                               creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    lines = []
    incoming = queue.Queue()

    def reader():
        for line in process.stdout:
            lines.append(line)
            incoming.put(line.strip())
        incoming.put(None)

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()
    deadline = time.monotonic() + args.timeout
    assertions = 0
    failure = None

    def send(command):
        process.stdin.write(command + "\n")
        process.stdin.flush()

    def wait(predicate, limit=15):
        end = min(deadline, time.monotonic() + limit)
        while time.monotonic() < end:
            try:
                line = incoming.get(timeout=max(0.01, end - time.monotonic()))
            except queue.Empty:
                break
            if line is None:
                raise ValueError("headless exited while waiting for a command result")
            if line.startswith("[FAIL]") or "Unknown command/expression" in line:
                raise ValueError(f"debugger command failed: {line}")
            if predicate(line):
                return line
        raise ValueError("timed out waiting for debugger state/result")

    try:
        expect_running = False
        for raw in script.splitlines():
            line = raw.strip()
            if line == "; WAIT_PAUSED":
                # GUI state updates may contain duplicate paused notifications.
                # A resume must first announce running; don't consume a stale
                # pause from the previous stop as completion of this command.
                if expect_running:
                    wait(lambda text: text == "[STATE] running")
                wait(lambda text: text == "[STATE] paused")
                expect_running = False
            elif line == "; WAIT_STOPPED":
                wait(lambda text: text == "[STATE] stopped")
            elif line == "; WAIT_SPIN":
                # Observe real target progress through a normal debugger
                # expression; do not sleep and assume the loop was entered.
                while True:
                    send('log "E2E SPIN {d:dword:[trace_party:gSpinIterations]}"')
                    result = wait(lambda text: text.startswith("E2E SPIN "))
                    if int(result.split()[-1]) >= 10:
                        assertions += 1
                        expect_running = False
                        break
                    time.sleep(0.05)
            elif line and not line.startswith(";"):
                send(line)
                if line == "run" or line.startswith("init "):
                    expect_running = True
                if line.startswith('log "E2E CHECK '):
                    result = wait(lambda text: text.startswith("E2E CHECK "))
                    assertions += 1
                    if result != "E2E CHECK 1":
                        raise ValueError(f"debugger expression failed: {line} -> {result}")
        send("exit")
        process.wait(timeout=10)
        if process.returncode != 0 or assertions == 0:
            raise ValueError(f"headless exit={process.returncode}, assertions={assertions}")
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        failure = str(error)
    finally:
        if process.poll() is None:
            # Kill only this test's process tree, not another debugger session.
            subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"], capture_output=True)
            process.wait(timeout=10)
        thread.join(timeout=5)
        (artifacts / "headless.stdout.txt").write_text("".join(lines), encoding="utf-8")
    if "TraceSetStepFilter system, run" in script and any("Run-to-party setup failed" in line for line in lines):
        failure = failure or "the fast-path pause test fell back to stepping"
    with log.open("a", encoding="utf-8") as stream:
        if failure:
            stream.write(f'\n[x64dbg-test] ASSERT FAIL source=driver message="{failure}"\n')
            stream.write(f"[x64dbg-test] FINAL status=fail asserts={assertions} reason=pause_e2e\n")
        else:
            stream.write(f"\n[x64dbg-test] FINAL status=pass asserts={assertions}\n")
    return int(failure is not None)
