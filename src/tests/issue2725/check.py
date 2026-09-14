from __future__ import annotations

import argparse
import sys
from pathlib import Path

THREAD_SWITCH_LOG = "Thread switched from"
PROCESS_STARTED = "Process Started:"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check that restarting the debuggee does not log a thread switch.")
    parser.add_argument("--log", required=True)
    parser.add_argument("--userdir", required=True)
    parser.add_argument("--runtime-dir", required=True)
    parser.add_argument("--artifacts-dir", required=True)
    return parser.parse_args()


def second_session_log(log_text: str) -> str | None:
    first = log_text.find(PROCESS_STARTED)
    if first < 0:
        return None
    second = log_text.find(PROCESS_STARTED, first + len(PROCESS_STARTED))
    if second < 0:
        return None
    return log_text[second:]


def main() -> int:
    args = parse_args()
    log_path = Path(args.log)
    log_text = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""
    session = second_session_log(log_text)
    if session is None:
        print("log does not contain two Process Started lines", file=sys.stderr)
        return 1
    if THREAD_SWITCH_LOG in session:
        print(f"found unexpected {THREAD_SWITCH_LOG!r} after restart", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
