from __future__ import annotations

import argparse
import sys
from pathlib import Path

THREAD_SWITCH_LOG = "Thread switched from"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check that restarting the debuggee does not log a thread switch.")
    parser.add_argument("--log", required=True)
    parser.add_argument("--userdir", required=True)
    parser.add_argument("--runtime-dir", required=True)
    parser.add_argument("--artifacts-dir", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    log_path = Path(args.log)
    log_text = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""
    if THREAD_SWITCH_LOG in log_text:
        print(f"found unexpected {THREAD_SWITCH_LOG!r} after restart", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
