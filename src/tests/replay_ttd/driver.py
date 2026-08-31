from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import threading
import time
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate deterministic TTD replay.")
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


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


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


def parse_position(line: str) -> tuple[int, int] | None:
    match = re.search(r"Replay position:\s*([0-9A-Fa-f]+):([0-9A-Fa-f]+)", line)
    return (int(match.group(1), 16), int(match.group(2), 16)) if match else None


def main() -> int:
    args = parse_args()
    headless = Path(args.headless).resolve()
    userdir = Path(args.userdir).resolve()
    artifacts = Path(args.artifacts_dir).resolve()
    log_path = Path(args.log).resolve()
    repo = Path(__file__).resolve().parents[3]
    arch = headless.parent.name.lower()
    fixture_root = Path(os.environ.get("X64DBG_TTD_FIXTURE_DIR", repo / "build" / "replay-ttd-fixtures"))
    trace = (fixture_root / arch / "replay_ttd.run").resolve()
    output_path = artifacts / "headless-output.txt"
    assertions: list[str] = []

    if args.engine != "DbgEng" or not trace.is_file():
        append_log(log_path, '[x64dbg-test] ASSERT FAIL source=driver message="TTD fixture or DbgEng engine unavailable"')
        append_log(log_path, "[x64dbg-test] FINAL status=fail asserts=1 reason=fixture_unavailable")
        return 1

    manifest_path = Path(__file__).with_name("fixture-manifest.json")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    fixture = manifest.get("fixtures", {}).get(arch, {})
    target = Path(args.debuggee).resolve()
    helper = target.with_name("replay_ttd_helper.dll")
    fixture_valid = (
        fixture.get("size") == trace.stat().st_size
        and fixture.get("sha256") == sha256_file(trace)
        and target.is_file()
        and fixture.get("targetSha256") == sha256_file(target)
        and helper.is_file()
        and fixture.get("helperSha256") == sha256_file(helper)
    )
    if not fixture_valid:
        append_log(log_path, '[x64dbg-test] ASSERT FAIL source=driver message="TTD fixture manifest or binary hashes do not match"')
        append_log(log_path, "[x64dbg-test] FINAL status=fail asserts=1 reason=fixture_hash_mismatch")
        return 1

    creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0) if args.no_console_window else 0
    debugger = Headless(headless, userdir, creationflags)

    def fail(reason: str, message: str) -> int:
        try:
            if debugger.process.poll() is None:
                debugger.send("stop")
                debugger.send("exit")
                debugger.process.wait(timeout=15)
        except Exception:
            debugger.process.kill()
            debugger.process.wait(timeout=5)
        output_path.write_text(debugger.output(), encoding="utf-8", errors="replace")
        append_log(log_path, f'[x64dbg-test] ASSERT FAIL source=driver message="{message}"')
        append_log(log_path, f"[x64dbg-test] FINAL status=fail asserts=1 reason={reason}")
        return 1

    timeout = max(45, args.timeout)
    if not debugger.wait_count("[headless] entering command loop", 1, timeout):
        return fail("headless_start", "headless command loop did not start")

    trace_arg = os.path.relpath(trace, headless.parent).replace("\\", "/")
    debugger.send(f"initreplay {trace_arg}")
    if not debugger.wait_count("System breakpoint reached!", 1, timeout):
        return fail("open", "TTD trace did not reach its synthetic startup pause")
    assertions.append("TTD bootstrap reached the synthetic system breakpoint")

    debugger.send("replaygetextent")
    if not debugger.wait_count("Replay extent:", 1, 15):
        return fail("extent", "TTD extent query failed")
    extent_line = debugger.matching("Replay extent:")[-1]
    extent_match = re.search(r"([0-9A-Fa-f]+):([0-9A-Fa-f]+)-([0-9A-Fa-f]+):([0-9A-Fa-f]+)", extent_line)
    if not extent_match:
        return fail("extent_parse", "TTD extent was not round-trippable")
    first = (int(extent_match.group(1), 16), int(extent_match.group(2), 16))
    last = (int(extent_match.group(3), 16), int(extent_match.group(4), 16))
    if first >= last:
        return fail("extent_order", "TTD extent is empty or reversed")
    assertions.append("TTD reported ordered first and last positions")

    debugger.send("log TTD_INITIAL_CIP={p:cip}")
    debugger.send("log TTD_INITIAL_PEB={p:peb()}")
    if not debugger.wait_count("TTD_INITIAL_PEB=", 1, 15):
        return fail("initial_state", "TTD initial context or PEB query failed")
    initial_lines = debugger.matching("TTD_INITIAL_CIP=") + debugger.matching("TTD_INITIAL_PEB=")
    if len(initial_lines) != 2 or any(line.rstrip().endswith("=0000000000000000") or line.rstrip().endswith("=00000000") for line in initial_lines):
        return fail("initial_state_zero", "TTD initial context or PEB was zero")
    assertions.append("TTD exposed initial context, memory, and PEB state")

    debugger.send("sti")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 1, 15):
        return fail("step_forward", "TTD forward step did not publish a position")
    forward_step = parse_position(debugger.matching("Replay position:")[-1])
    if not forward_step or forward_step <= first:
        return fail("step_forward_position", "TTD forward step did not advance")
    debugger.send("replaystepback")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 2, 15):
        return fail("step_reverse", "TTD reverse step did not publish a position")
    reverse_step = parse_position(debugger.matching("Replay position:")[-1])
    if reverse_step != first:
        return fail("step_reverse_position", "TTD reverse step did not restore the first position")
    debugger.send("sto")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 3, 15):
        return fail("step_over", "TTD step-over did not publish a position")
    step_over = parse_position(debugger.matching("Replay position:")[-1])
    if not step_over or step_over <= first:
        return fail("step_over_position", "TTD step-over did not advance")
    debugger.send(f"replaysetposition {first[0]:X}:{first[1]:X}")
    if not debugger.wait_count("Replay position set:", 1, 15):
        return fail("step_restore", "TTD could not restore the first position after step-over")
    assertions.append("TTD step-into, step-over, and reverse-step navigation passed")

    debugger.send("bp replay_ttd.ReplayTtdMilestone")
    if not debugger.wait_count("Breakpoint at", 1, 15):
        return fail("code_breakpoint_set", "TTD logical code breakpoint was not set")
    debugger.send("run")
    if not debugger.wait_count("ReplayTtdMilestone>", 1, timeout):
        return fail("code_breakpoint_forward", "TTD forward logical code breakpoint did not hit")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 4, 15):
        return fail("code_position", "TTD code breakpoint position was unavailable")
    forward_hit = parse_position(debugger.matching("Replay position:")[-1])
    if not forward_hit or not (first < forward_hit < last):
        return fail("code_position_range", "TTD code breakpoint position is outside the trace")
    assertions.append("TTD logical code breakpoint hit during forward execution")

    debugger.send("run")
    if not debugger.wait_count("ReplayTtdMilestone>", 2, timeout):
        return fail("code_breakpoint_repeat", "TTD repeated logical code breakpoint did not hit")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 5, 15):
        return fail("repeat_position", "TTD repeated breakpoint position was unavailable")
    repeated_hit = parse_position(debugger.matching("Replay position:")[-1])
    if not repeated_hit or repeated_hit <= forward_hit:
        return fail("repeat_position_order", "TTD repeated breakpoint did not advance")
    assertions.append("TTD found repeated execution of the deterministic function")

    debugger.send(f"replaysetposition {last[0]:X}:{last[1]:X}")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 6, 15) or parse_position(debugger.matching("Replay position:")[-1]) != last:
        return fail("exact_last", "TTD exact seek did not reach the last position")
    debugger.send(f"replaysetposition {repeated_hit[0]:X}:{repeated_hit[1]:X}")
    if not debugger.wait_count("Replay position set:", 3, 15):
        return fail("reverse_setup_seek", "TTD could not restore the repeated breakpoint position")
    debugger.send("bc replay_ttd.ReplayTtdMilestone")
    paused_before = len(debugger.matching("[STATE] paused"))
    debugger.send("sti 20")
    if not debugger.wait_count("[STATE] paused", paused_before + 1, 30):
        return fail("reverse_setup_step", "TTD repeated stepping did not complete")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 7, 15):
        return fail("reverse_setup", "TTD could not step beyond the repeated breakpoint")
    debugger.send("bp replay_ttd.ReplayTtdMilestone")
    if not debugger.wait_count("Breakpoint at", 2, 15):
        return fail("reverse_breakpoint_set", "TTD reverse logical code breakpoint was not set")
    debugger.send("replayrunback")
    if not debugger.wait_count("ReplayTtdMilestone>", 3, timeout):
        return fail("code_breakpoint_reverse", "TTD reverse logical code breakpoint did not hit")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 8, 15):
        return fail("reverse_position", "TTD reverse breakpoint position was unavailable")
    reverse_hit = parse_position(debugger.matching("Replay position:")[-1])
    if reverse_hit != repeated_hit:
        return fail("reverse_position_range", "TTD reverse breakpoint did not return to the prior exact position")
    assertions.append("TTD exact seek and reverse logical code breakpoint execution passed")

    debugger.send("run")
    if not debugger.wait_count("ReplayTtdMilestone>", 4, timeout):
        return fail("module_milestone", "TTD did not reach the helper-module milestone")
    if not debugger.wait_count("replay_ttd_helper.dll", 1, 15):
        return fail("module_load", "TTD helper-module load was not exposed")
    debugger.send(f"replaysetposition {last[0]:X}:{last[1]:X}")
    if not debugger.wait_count("replay_ttd_helper.dll", 2, 15):
        return fail("module_unload", "TTD helper-module unload was not exposed")
    assertions.append("TTD exposed forward and backward module/thread timeline changes")

    debugger.send("bc replay_ttd.ReplayTtdMilestone")
    debugger.send(f"replaysetposition {first[0]:X}:{first[1]:X}")
    debugger.send("bpmrange replay_ttd.gReplayTtdState, 4, w")
    if not debugger.wait_count("Memory breakpoint at", 1, 15):
        return fail("data_breakpoint_set", "TTD logical data breakpoint was not set")
    debugger.send("run")
    if not debugger.wait_count("gReplayTtdState>", 1, timeout):
        return fail("data_breakpoint_hit", "TTD logical write breakpoint did not hit")
    debugger.send("log TTD_STAGE_BEFORE={d:[replay_ttd.gReplayTtdState]}")
    if not debugger.wait_count("TTD_STAGE_BEFORE=", 1, 15):
        return fail("data_state_before", "TTD write breakpoint did not expose pre-write state")
    before_stage = debugger.matching("TTD_STAGE_BEFORE=")[-1].rsplit("=", 1)[-1]
    if before_stage not in {"0", "1"}:
        return fail("data_state_before_value", "TTD pre-write state was unexpected")
    debugger.send("bpmc replay_ttd.gReplayTtdState")
    debugger.send("sti")
    debugger.send("log TTD_STAGE_AFTER={d:[replay_ttd.gReplayTtdState]}")
    if not debugger.wait_count("TTD_STAGE_AFTER=1", 1, 15):
        return fail("data_state_after", "TTD post-write state was not observable")
    assertions.append("TTD logical data breakpoint exposed known pre-write and post-write state")

    debugger.send(f"replaysetposition {first[0]:X}:{first[1]:X}")
    debugger.send("run")
    if not debugger.wait_count("First chance exception", 1, timeout) or not debugger.wait_count("E0424242", 1, 15):
        return fail("exception", "TTD handled exception was not exposed")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", 9, 15):
        return fail("exception_position", "TTD handled-exception position was unavailable")
    exception_position = parse_position(debugger.matching("Replay position:")[-1])
    if not exception_position or not (first < exception_position < last):
        return fail("exception_position_range", "TTD handled-exception position is outside the trace")
    assertions.append("TTD exposed the deterministic handled exception and its exact position")

    debugger.send("stop")
    if not debugger.wait_count("Debugging stopped!", 1, 30):
        return fail("stop", "TTD session did not stop cleanly")
    for iteration in range(2, 21):
        debugger.send(f"initreplay {trace_arg}")
        if not debugger.wait_count("System breakpoint reached!", iteration, timeout):
            return fail("reopen", f"TTD trace did not reopen for session {iteration}")
        debugger.send("stop")
        if not debugger.wait_count("Debugging stopped!", iteration, 45):
            return fail("restop", f"TTD session {iteration} did not stop cleanly")
    assertions.append("TTD passed twenty-session open and teardown stress")

    debugger.send("exit")
    try:
        debugger.process.wait(timeout=30)
    except subprocess.TimeoutExpired:
        return fail("exit", "headless process did not exit")
    output_path.write_text(debugger.output(), encoding="utf-8", errors="replace")
    if debugger.process.returncode != 0:
        return fail("process_exit", f"headless exited with {debugger.process.returncode}")

    for message in assertions:
        append_log(log_path, f'[x64dbg-test] ASSERT PASS source=driver message="{message}"')
    append_log(log_path, f"[x64dbg-test] FINAL status=pass asserts={len(assertions)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
