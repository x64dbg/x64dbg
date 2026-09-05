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

    def mark(self) -> int:
        with self.condition:
            return len(self.lines)

    def wait_ordered_after(self, first: str, second: str, start: int, timeout: float) -> bool:
        deadline = time.monotonic() + timeout
        with self.condition:
            while True:
                first_index = next((i for i in range(start, len(self.lines)) if first in self.lines[i]), None)
                if first_index is not None and any(second in self.lines[i] for i in range(first_index + 1, len(self.lines))):
                    return True
                if self.process.poll() is not None:
                    return False
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return False
                self.condition.wait(min(remaining, 0.25))

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
    output_arch = headless.parent.name.lower()
    arch = "x64" if output_arch.startswith("x64") else "x32" if output_arch.startswith("x32") else output_arch
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

    def navigate_and_query(command: str, movement_timeout: float = 30) -> tuple[int, int] | None:
        state_mark = debugger.mark()
        positions_before = len(debugger.matching("Replay position:"))
        debugger.send(command)
        if not debugger.wait_ordered_after("[STATE] running", "[STATE] paused", state_mark, movement_timeout):
            return None
        debugger.send("replaygetposition")
        if not debugger.wait_count("Replay position:", positions_before + 1, 15):
            return None
        return parse_position(debugger.matching("Replay position:")[-1])

    def seek_and_wait(position: tuple[int, int]) -> bool:
        seeks_before = len(debugger.matching("Replay position set:"))
        debugger.send(f"replaysetposition {position[0]:X}:{position[1]:X}")
        return debugger.wait_count("Replay position set:", seeks_before + 1, 15)

    def query_thread_instruction(marker: str) -> tuple[int, str] | None:
        debugger.send(f"log {marker} TID={{d:tid()}} INS={{i:cip}}")
        if not debugger.wait_count(marker, 1, 15):
            return None
        line = debugger.matching(marker)[-1]
        match = re.search(r"TID=(\d+).*INS=(.*)$", line)
        return (int(match.group(1)), match.group(2).strip()) if match else None

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
    debugger.send("log TTD_INITIAL_STACK_VALID={d:mem.valid(csp)}")
    if not debugger.wait_count("TTD_INITIAL_PEB=", 1, 15) or not debugger.wait_count("TTD_INITIAL_STACK_VALID=1", 1, 15):
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
    if not seek_and_wait(first):
        return fail("step_restore", "TTD could not restore the first position after step-over")
    if navigate_and_query("sto 2") != (first[0], first[1] + 2):
        return fail("step_over_repeat", "TTD repeated step-over did not use the standard step-repeat path")
    if not seek_and_wait(first):
        return fail("step_over_repeat_restore", "TTD could not restore the first position after repeated step-over")

    # Probe the beginning boundary and rapidly alternate directions before any
    # logical breakpoint has been installed.
    reverse_errors_before = len(debugger.matching("Unable to reverse step"))
    running_before_first_step = len(debugger.matching("[STATE] running"))
    debugger.send("replaystepback")
    if not debugger.wait_count("Unable to reverse step", reverse_errors_before + 1, 15):
        return fail("first_reverse_step", "TTD did not reject a reverse step before the first position")
    time.sleep(0.1)
    if len(debugger.matching("[STATE] running")) != running_before_first_step:
        return fail("first_reverse_resume", "A rejected first-position reverse step unexpectedly resumed replay")
    first_transition_cases = [
        ("sti", (first[0], first[1] + 1)),
        ("sti", (first[0], first[1] + 2)),
        ("replaystepback", (first[0], first[1] + 1)),
        ("sti", (first[0], first[1] + 2)),
        ("replaystepback", (first[0], first[1] + 1)),
        ("replaystepback", first),
    ]
    for command, expected_position in first_transition_cases:
        if navigate_and_query(command) != expected_position:
            return fail("first_transition_matrix", "TTD failed an alternating step transition near the first position")
    if navigate_and_query("replayrunback", timeout) != first:
        return fail("first_reverse_run", "TTD reverse run at the first position did not retain its boundary")

    # The fixture begins in LdrInitializeThunk and reaches a call shared by
    # several thread initializations. Step-over must reach the return site on
    # the selected thread, not the first peer thread that executes that address.
    call_position = first
    call_state = None
    for probe_index in range(32):
        call_state = query_thread_instruction(f"TTD_STO_THREAD_BEFORE_{probe_index}=")
        if not call_state:
            return fail("thread_step_over_probe", "TTD could not query the thread and instruction before step-over")
        if call_state[1].lower().startswith("call "):
            break
        next_position = navigate_and_query("sti")
        if not next_position or next_position <= call_position:
            return fail("thread_step_over_seek", "TTD could not step to the loader call used for thread-affinity validation")
        call_position = next_position
    else:
        return fail("thread_step_over_call", "TTD did not find the expected loader call near the first position")
    step_over_position = navigate_and_query("sto", timeout)
    after_call_state = query_thread_instruction("TTD_STO_THREAD_AFTER=")
    if not step_over_position or step_over_position <= call_position or not after_call_state:
        return fail("thread_step_over", "TTD current-thread step-over did not reach a valid return position")
    if after_call_state[0] != call_state[0]:
        return fail("thread_step_over_affinity", "TTD step-over completed on a different thread")
    if not seek_and_wait(first):
        return fail("thread_step_over_restore", "TTD could not restore the first position after thread-affinity validation")
    assertions.append("TTD first-boundary, alternating steps, and current-thread step-over passed")

    debugger.send("bp replay_ttd.ReplayTtdMilestone")
    if not debugger.wait_count("Breakpoint at", 1, 15):
        return fail("code_breakpoint_set", "TTD logical code breakpoint was not set")
    debugger.send("run")
    if not debugger.wait_count("ReplayTtdMilestone>", 1, timeout):
        return fail("code_breakpoint_forward", "TTD forward logical code breakpoint did not hit")
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("code_position", "TTD code breakpoint position was unavailable")
    forward_hit = parse_position(debugger.matching("Replay position:")[-1])
    if not forward_hit or not (first < forward_hit < last):
        return fail("code_position_range", "TTD code breakpoint position is outside the trace")
    assertions.append("TTD logical code breakpoint hit during forward execution")

    debugger.send("run")
    if not debugger.wait_count("ReplayTtdMilestone>", 2, timeout):
        return fail("code_breakpoint_repeat", "TTD repeated logical code breakpoint did not hit")
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("repeat_position", "TTD repeated breakpoint position was unavailable")
    repeated_hit = parse_position(debugger.matching("Replay position:")[-1])
    if not repeated_hit or repeated_hit <= forward_hit:
        return fail("repeat_position_order", "TTD repeated breakpoint did not advance")
    assertions.append("TTD found repeated execution of the deterministic function")

    position_count = len(debugger.matching("Replay position:"))
    debugger.send(f"replaysetposition {last[0]:X}:{last[1]:X}")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15) or parse_position(debugger.matching("Replay position:")[-1]) != last:
        return fail("exact_last", "TTD exact seek did not reach the last position")
    if not seek_and_wait(repeated_hit):
        return fail("reverse_setup_seek", "TTD could not restore the repeated breakpoint position")
    debugger.send("bc replay_ttd.ReplayTtdMilestone")
    paused_before = len(debugger.matching("[STATE] paused"))
    debugger.send("sti 20")
    if not debugger.wait_count("[STATE] paused", paused_before + 1, 30):
        return fail("reverse_setup_step", "TTD repeated stepping did not complete")
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("reverse_setup", "TTD could not step beyond the repeated breakpoint")
    debugger.send("bp replay_ttd.ReplayTtdMilestone")
    if not debugger.wait_count("Breakpoint at", 2, 15):
        return fail("reverse_breakpoint_set", "TTD reverse logical code breakpoint was not set")
    debugger.send("replayrunback")
    if not debugger.wait_count("ReplayTtdMilestone>", 3, timeout):
        return fail("code_breakpoint_reverse", "TTD reverse logical code breakpoint did not hit")
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("reverse_position", "TTD reverse breakpoint position was unavailable")
    reverse_hit = parse_position(debugger.matching("Replay position:")[-1])
    if reverse_hit != repeated_hit:
        return fail("reverse_position_range", "TTD reverse breakpoint did not return to the prior exact position")

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
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("data_position", "TTD write-breakpoint position was unavailable")
    data_hit = parse_position(debugger.matching("Replay position:")[-1])
    data_after = navigate_and_query("sti")
    if not data_hit or not data_after or data_after <= data_hit:
        return fail("data_step_forward", "TTD did not step forward away from a persistent data breakpoint")
    debugger.send("log TTD_STAGE_AFTER={d:[replay_ttd.gReplayTtdState]}")
    if not debugger.wait_count("TTD_STAGE_AFTER=1", 1, 15):
        return fail("data_state_after", "TTD post-write state was not observable")
    if navigate_and_query("replaystepback") != data_hit:
        return fail("data_step_reverse_hit", "TTD reverse step did not return to the persistent data-breakpoint hit")
    data_before = navigate_and_query("replaystepback")
    if not data_before or data_before >= data_hit:
        return fail("data_step_reverse_away", "TTD reverse step did not leave the persistent data-breakpoint hit")
    if navigate_and_query("sti") != data_hit:
        return fail("data_step_forward_hit", "TTD forward step did not return to the persistent data-breakpoint hit")
    if navigate_and_query("sti") != data_after:
        return fail("data_step_forward_away", "TTD repeated forward step from a persistent data breakpoint was not deterministic")
    debugger.send("bpmc replay_ttd.gReplayTtdState")
    assertions.append("TTD persistent data-breakpoint forward/reverse step transitions passed")

    debugger.send(f"replaysetposition {first[0]:X}:{first[1]:X}")
    debugger.send("run")
    if not debugger.wait_count("First chance exception", 1, timeout) or not debugger.wait_count("E0424242", 1, 15):
        return fail("exception", "TTD handled exception was not exposed")
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("exception_position", "TTD handled-exception position was unavailable")
    exception_position = parse_position(debugger.matching("Replay position:")[-1])
    if not exception_position or not (first < exception_position < last):
        return fail("exception_position_range", "TTD handled-exception position is outside the trace")
    debugger.send("bp replay_ttd.entry")
    if not debugger.wait_count("Breakpoint at", 3, 15):
        return fail("long_reverse_breakpoint_set", "TTD could not set the early entry breakpoint")
    debugger.send("replayrunback")
    if not debugger.wait_count("INT3 breakpoint at", 5, timeout):
        return fail("long_reverse_breakpoint", "TTD reverse run did not reach the early entry point from the exception")
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("long_reverse_position", "TTD long reverse-run position was unavailable")
    long_reverse_position = parse_position(debugger.matching("Replay position:")[-1])
    if not long_reverse_position or not (first < long_reverse_position < exception_position):
        return fail("long_reverse_position_range", "TTD long reverse run did not move before the exception")

    # A forward single-step from a reverse-hit persistent code breakpoint must
    # leave that exact execution, rather than resume all the way to the next
    # recorded exception because the cursor reports the current watchpoint
    # again as a stale zero-step hit.
    paused_before_reverse_hit_step = len(debugger.matching("[STATE] paused"))
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("sti")
    if not debugger.wait_count("[STATE] paused", paused_before_reverse_hit_step + 1, 30):
        return fail("reverse_hit_step", "TTD did not pause after stepping forward from a reverse breakpoint hit")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("reverse_hit_step_position", "TTD forward-step position after a reverse breakpoint hit was unavailable")
    reverse_hit_step = parse_position(debugger.matching("Replay position:")[-1])
    breakpoint_before = (long_reverse_position[0], long_reverse_position[1] - 1)
    breakpoint_after = (long_reverse_position[0], long_reverse_position[1] + 1)
    if reverse_hit_step != breakpoint_after:
        return fail("reverse_hit_step_range", "TTD forward step from a reverse breakpoint hit did not advance exactly one execution")

    # Exercise direction and movement-mode transitions around a persistent
    # logical code breakpoint. This covers stepping onto and away from the hit
    # in both directions, run/step switches, and repeated exception round trips.
    transition_cases = [
        ("replaystepback", long_reverse_position, 30, "reverse step onto the persistent breakpoint"),
        ("replaystepback", breakpoint_before, 30, "reverse step away from the persistent breakpoint"),
        ("sti", long_reverse_position, 30, "forward step onto the persistent breakpoint"),
        ("sti", breakpoint_after, 30, "forward step away from the persistent breakpoint"),
        ("run", exception_position, timeout, "forward run after alternating breakpoint steps"),
        ("replayrunback", long_reverse_position, timeout, "reverse run from the exception to the breakpoint"),
        ("sti", breakpoint_after, 30, "forward step after reverse run"),
        ("replayrunback", long_reverse_position, timeout, "reverse run immediately after a forward step"),
        ("replaystepback", breakpoint_before, 30, "reverse step immediately after reverse run"),
        ("run", long_reverse_position, timeout, "forward run from before the breakpoint"),
        ("run", exception_position, timeout, "forward run from the breakpoint to the exception"),
        ("replayrunback", long_reverse_position, timeout, "second exception-to-breakpoint reverse run"),
    ]
    for command, expected_position, movement_timeout, description in transition_cases:
        actual_position = navigate_and_query(command, movement_timeout)
        if actual_position != expected_position:
            return fail(
                "transition_matrix",
                f"TTD {description} reached {actual_position!r}, expected {expected_position!r}",
            )

    step_over_after_reverse = navigate_and_query("sto", timeout)
    if not step_over_after_reverse or not (long_reverse_position < step_over_after_reverse < exception_position):
        return fail("transition_step_over", "TTD step-over after a reverse run did not advance to a valid position")
    if navigate_and_query("replayrunback", timeout) != long_reverse_position:
        return fail("transition_step_over_runback", "TTD reverse run after step-over did not return to the persistent breakpoint")
    if navigate_and_query("sto", timeout) != step_over_after_reverse:
        return fail("transition_step_over_repeat", "TTD repeated step-over after reverse run was not deterministic")
    step_over_previous = navigate_and_query("replaystepback")
    if not step_over_previous or not (long_reverse_position <= step_over_previous < step_over_after_reverse):
        return fail("transition_step_over_back", "TTD reverse step after step-over reached an invalid position")
    if navigate_and_query("sti") != step_over_after_reverse:
        return fail("transition_step_over_forward", "TTD forward step did not restore the step-over destination")
    if navigate_and_query("replayrunback", timeout) != long_reverse_position:
        return fail("transition_step_over_final_runback", "TTD final reverse run did not restore the persistent breakpoint")
    assertions.append("TTD mixed forward/reverse step, step-over, and run transition matrix passed")
    debugger.send("bc replay_ttd.entry")
    if not seek_and_wait(first):
        return fail("interrupt_setup", "TTD could not seek for the interrupt test")
    system_breaks_before_interrupt = len(debugger.matching("System breakpoint reached!"))
    debugger.send("run")
    debugger.send("pause")
    if not debugger.wait_count("System breakpoint reached!", system_breaks_before_interrupt + 1, 30):
        return fail("interrupt", "TTD replay did not respond to an explicit pause")
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("interrupt_position", "TTD interrupted position was unavailable")
    interrupted_position = parse_position(debugger.matching("Replay position:")[-1])
    if not interrupted_position or not (first <= interrupted_position <= exception_position):
        return fail("interrupt_position_range", "TTD pause did not preserve a valid cursor position")
    if not seek_and_wait(exception_position):
        return fail("pseudo_exit_setup", "TTD could not seek to the handled exception for the exit-boundary test")
    exceptions_before_exit_run = len(debugger.matching("First chance exception"))
    debugger.send("run")
    exit_deadline = time.monotonic() + timeout
    exception_resumed = False
    while not debugger.matching("Replay reached the recorded process exit.") and time.monotonic() < exit_deadline:
        if not exception_resumed and len(debugger.matching("First chance exception")) > exceptions_before_exit_run:
            # Some x86 cursor selections re-publish the exception once when
            # resumed from its exact event position. Continue recorded state.
            debugger.send("run")
            exception_resumed = True
        time.sleep(0.05)
    if not debugger.matching("Replay reached the recorded process exit."):
        return fail("pseudo_exit", "TTD did not publish its recorded process-exit boundary")
    if debugger.matching("Debugging stopped!"):
        return fail("pseudo_exit_teardown", "TTD process-exit boundary unexpectedly destroyed the session")
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("pseudo_exit_position", "TTD process-exit boundary position was unavailable")
    exit_position = parse_position(debugger.matching("Replay position:")[-1])
    expected_exit_text = fixture.get("processExitBoundary", "")
    expected_exit_match = re.fullmatch(r"([0-9A-Fa-f]+):([0-9A-Fa-f]+)", expected_exit_text)
    expected_exit = (int(expected_exit_match.group(1), 16), int(expected_exit_match.group(2), 16)) if expected_exit_match else None
    if not exit_position or not (exception_position < exit_position <= last) or exit_position != expected_exit:
        return fail("pseudo_exit_position_range", "TTD process-exit boundary did not match its fixture manifest")
    debugger.send("log TTD_EXIT_STACK_VALID={d:mem.valid(csp)}")
    debugger.send("log TTD_EXIT_STACK_VALUE={p:[csp]}")
    if not debugger.wait_count("TTD_EXIT_STACK_VALID=1", 1, 15) or not debugger.wait_count("TTD_EXIT_STACK_VALUE=", 1, 15):
        return fail("pseudo_exit_stack", "TTD process-exit boundary did not retain readable stack memory")

    # A normal forward step must not cross the retained pseudo-exit boundary
    # into TTD's sparse raw post-exit cursor. Reverse navigation remains valid,
    # and stepping forward from the prior instruction may return to the boundary.
    step_errors_before = len(debugger.matching("Unable to step forward in this TTD position"))
    running_before_exit_step = len(debugger.matching("[STATE] running"))
    position_count = len(debugger.matching("Replay position:"))
    debugger.send("sti")
    if not debugger.wait_count("Unable to step forward in this TTD position", step_errors_before + 1, 15):
        return fail("pseudo_exit_step", "TTD allowed or stranded a forward step beyond the pseudo-exit boundary")
    time.sleep(0.1)
    if len(debugger.matching("[STATE] running")) != running_before_exit_step:
        return fail("pseudo_exit_step_resume", "A rejected pseudo-exit step unexpectedly resumed replay")
    debugger.send("replaygetposition")
    if not debugger.wait_count("Replay position:", position_count + 1, 15):
        return fail("pseudo_exit_step_position", "TTD pseudo-exit position was unavailable after a rejected step")
    if parse_position(debugger.matching("Replay position:")[-1]) != exit_position:
        return fail("pseudo_exit_step_crossed", "TTD forward step crossed the retained pseudo-exit boundary")

    exit_previous = (exit_position[0], exit_position[1] - 1)
    if navigate_and_query("replaystepback") != exit_previous:
        return fail("pseudo_exit_stepback", "TTD could not reverse-step away from its pseudo-exit boundary")
    if navigate_and_query("sti") != exit_position:
        return fail("pseudo_exit_step_return", "TTD could not step forward back to its pseudo-exit boundary")

    if not seek_and_wait(first):
        return fail("pseudo_exit_seek", "TTD could not seek backward while paused at process exit")
    assertions.append("TTD handled interruption and enforced retained process-exit boundary navigation")

    debugger.send("stop")
    if not debugger.wait_count("Debugging stopped!", 1, 30):
        return fail("stop", "TTD session did not stop cleanly")
    system_breaks_before_reopen = len(debugger.matching("System breakpoint reached!"))
    for iteration in range(2, 21):
        debugger.send(f"initreplay {trace_arg}")
        expected_system_breaks = system_breaks_before_reopen + iteration - 1
        if not debugger.wait_count("System breakpoint reached!", expected_system_breaks, timeout):
            return fail("reopen", f"TTD trace did not reopen for session {iteration}")
        # The message is emitted immediately before the callback acquires the
        # run lock. Give that short handoff time to complete before requesting
        # teardown; a paused-state notification may be coalesced with the
        # earlier initialization state and is therefore not a reliable marker.
        time.sleep(0.25)
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
