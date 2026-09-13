"""Launch ordinary debugger scripts, then validate their real trace artifacts.

No plugin, internal API calls, synthesized trace records, or emulated stepping.
Only @TARGET@/@TRACE@/@TRACELOG@ path substitution is performed on the scripts.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

FINAL = re.compile(r"^\[x64dbg-test\] FINAL status=pass asserts=([1-9][0-9]*)$", re.M)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("headless", "debuggee", "script", "runtime-dir", "userdir", "log", "artifacts-dir", "engine"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--timeout", required=True, type=int)
    parser.add_argument("--no-console-window", action="store_true")
    return parser.parse_args()


def trace_addresses(path: Path) -> list[int]:
    """Strict sequential decoder of docs/developers/tracefile.md (not a writer)."""
    data = path.read_bytes()
    offset = 0

    def take(size):
        nonlocal offset
        if offset + size > len(data):
            raise ValueError(f"truncated trace block at {offset:#x}")
        result = data[offset:offset + size]
        offset += size
        return result

    if take(4) != b"TRAC":
        raise ValueError("missing TRAC magic")
    header = json.loads(take(int.from_bytes(take(4), "little")))
    if header.get("arch") not in ("x86", "x64") or header.get("ver") != 1:
        raise ValueError(f"unsupported trace header: {header}")
    width = 8 if header["arch"] == "x64" else 4
    cip_index = 16 if width == 8 else 8  # REGISTERCONTEXT in bridgemain.h
    registers = {}
    thread = None
    addresses = []
    while offset < len(data):
        block_type = take(1)[0]
        if block_type >= 0x80:
            take(int.from_bytes(take(4), "little"))
            continue
        if block_type != 0:
            raise ValueError(f"unexpected trace block type {block_type:#x}")
        changes, accesses, flags = take(3)
        if flags & 0x80:
            thread = int.from_bytes(take(4), "little")
        if not thread or flags & 0x70 or not (flags & 0xF):
            raise ValueError("invalid thread/opcode flags in trace")
        take(flags & 0xF)
        positions = take(changes)
        index = -1
        for delta in positions:
            index += delta + 1
            registers[index] = int.from_bytes(take(width), "little")
        if cip_index not in registers:
            raise ValueError("trace starts without an instruction pointer")
        addresses.append(registers[cip_index])
        memory_flags = take(accesses)
        if any(flag & ~1 for flag in memory_flags):
            raise ValueError("unsupported memory access flags")
        take(width * accesses * 2)  # addresses and old values
        take(width * sum(not (flag & 1) for flag in memory_flags))
    if not addresses:
        raise ValueError("empty instruction trace")
    return addresses


def check_recording(trace: Path, trace_log: Path, debug_log: str, script: str):
    start = re.search(r"^E2E START ([0-9A-Fa-f]+) ([01])\s*$", debug_log, re.M)
    if start is None:
        raise ValueError("missing start-address marker")
    selected = re.search(r"^TraceSetStepFilter (user|system)", script, re.M)
    if selected is None:
        raise ValueError("missing explicit party in test script")
    party = int(selected[1] == "system")
    entries = []
    for line in trace_log.read_text(encoding="utf-8").splitlines():
        fields = re.fullmatch(r"([0-9A-Fa-f]+) ([01])", line.strip())
        if fields is None or int(fields[2]) != party:
            raise ValueError(f"unexpected/excluded text-log instruction: {line!r}")
        entries.append(int(fields[1], 16))
    if len(entries) < 2:
        raise ValueError("too few logged instructions to exercise recording")
    # The recorder queues the starting instruction before the first step; the
    # last (stop) instruction is pending, not executed, when recording closes.
    history = [int(address, 16) for address in re.findall(r"^E2E HISTORY ([0-9A-Fa-f]+)\s*$", debug_log, re.M)]
    expected = history + ([int(start[1], 16)] if int(start[2]) == party else []) + entries[:-1]
    recorded = trace_addresses(trace)
    if recorded != expected:
        mismatch = next((i for i, pair in enumerate(zip(recorded, expected)) if pair[0] != pair[1]), min(len(recorded), len(expected)))
        raise ValueError(f"binary/text trace mismatch at index {mismatch}: "
                         f"recorded={recorded[mismatch:mismatch+4]}, expected={expected[mismatch:mismatch+4]}; "
                         f"lengths {len(recorded)} vs {len(expected)}")
    return len(recorded)


def write_runtime_metadata(args, artifacts: Path, headless: Path):
    engine = headless.parent / ("TitanEngine.dll" if args.engine == "TitanEngine" else f"{args.engine}/TitanEngine.dll")
    images = [headless, Path(args.debuggee).resolve(), engine]
    images += [headless.parent / name for name in ("x64dbg.dll", "x32dbg.dll", "x64bridge.dll", "x32bridge.dll")
               if (headless.parent / name).is_file()]
    metadata = {"engine": args.engine, "images": {}}
    for image in images:
        digest = hashlib.sha256()
        with image.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
        metadata["images"][str(image)] = digest.hexdigest()
    (artifacts / "runtime.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")


def main():
    args = parse_args()
    artifacts = Path(args.artifacts_dir).resolve()
    log = Path(args.log).resolve()
    headless = Path(args.headless).resolve()
    write_runtime_metadata(args, artifacts, headless)
    source = Path(args.script).read_text(encoding="utf-8")
    trace = artifacts / "record.trace"
    trace_log = artifacts / "instructions.log"
    script = source.replace("@TARGET@", str(Path(args.debuggee).resolve()))
    script = script.replace("@TRACE@", str(trace)).replace("@TRACELOG@", str(trace_log))
    generated = artifacts / "test.txt"
    generated.write_text(script, encoding="utf-8")
    if "; INTERACTIVE_PAUSE" in source:
        from pause_driver import run
        return run(args, script)
    command = [str(headless), "-testing", "-userdir", str(Path(args.userdir).resolve()),
               "-c", f'RedirectLog "{log}"', "-cf", str(generated)]
    process = subprocess.Popen(command, cwd=headless.parent, stdin=subprocess.PIPE,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    try:
        output, _ = process.communicate(timeout=args.timeout)
        (artifacts / "headless.stdout.txt").write_bytes(output)
        debug_log = log.read_text(encoding="utf-8", errors="replace") if log.exists() else ""
        if process.returncode != 0 or FINAL.search(debug_log) is None:
            raise ValueError(f"headless/script failed: exit={process.returncode}; see headless.stdout.txt")
        if "@TRACE@" in source:
            count = check_recording(trace, trace_log, debug_log, script)
            print(f"Validated {count} real binary records against the debugger text log", flush=True)
        fallbacks = debug_log.count("Run-to-party setup failed")
        if "; EXPECT_FALLBACK" in source:
            if fallbacks != 1:
                raise ValueError("expected exactly one explicit fallback diagnostic")
        elif re.search(r"^TraceSetStepFilter (user|system), run$", script, re.M) and fallbacks:
            raise ValueError("fast-path test silently fell back to stepping")
        if "; EXPECT_EMPTY_LOG" in source and trace_log.read_bytes():
            raise ValueError("false log condition still produced trace log text")
        if "; EXPECT_MODULE_CHANGE" in source:
            for event in ("Loaded", "Unloaded"):
                if not re.search(rf"DLL {event}: [^\r\n]*version\.dll", debug_log, re.I):
                    raise ValueError(f"missing real DLL {event} event for version.dll")
        return 0
    except (ValueError, OSError, subprocess.TimeoutExpired) as error:
        if process.poll() is None:
            subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"], capture_output=True)
            output, _ = process.communicate()
            (artifacts / "headless.stdout.txt").write_bytes(output)
        print(str(error), file=sys.stderr)
        with log.open("a", encoding="utf-8") as stream:
            stream.write(f'\n[x64dbg-test] ASSERT FAIL source=driver message="{error}"\n')
            stream.write("[x64dbg-test] FINAL status=fail asserts=1 reason=trace_e2e\n")
        return 1


if __name__ == "__main__":
    sys.exit(main())
