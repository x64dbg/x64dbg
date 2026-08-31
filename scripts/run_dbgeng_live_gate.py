#!/usr/bin/env python3
"""Run the repeatable x64/x32 DbgEng live-user-mode completion gate."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def run(command: list[str], cwd: Path) -> None:
    print("+", subprocess.list2cmdline(command), flush=True)
    subprocess.run(command, cwd=cwd, check=True)


def discover_membp_tests(root: Path) -> list[str]:
    result = []
    for script in sorted((root / "src/tests/membp").glob("test*.txt")):
        variant = script.stem.removeprefix("test").removeprefix(".")
        result.append("membp" + (f"/{variant}" if variant else ""))
    return result


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--artifacts-dir", type=Path, default=root / "build/dbgeng-live-gate")
    parser.add_argument("--skip-cross-engine", action="store_true")
    args = parser.parse_args()
    if args.repeats < 1:
        parser.error("--repeats must be positive")

    python = sys.executable
    run([python, "-m", "py_compile", "scripts/check_titanengine_exports.py", "src/tests/run.py"], root)
    run([python, "scripts/check_titanengine_exports.py"], root)
    run(["git", "diff", "--check"], root)
    run(["git", "-C", str(root.parent / "x64dbg-dbgeng"), "-c", "core.whitespace=cr-at-eol", "diff", "--check"], root)

    live_tests = [
        "attach_pause/breakin",
        "script_run_exit/memory",
        "script_run_exit/multi-session",
        "script_run_exit/lifecycle-stress",
        "swbp_stale",
        "swbp_stale/step",
        "swbp_stale/long",
        "swbp_stale/ud2",
        *discover_membp_tests(root),
    ]
    for iteration in range(1, args.repeats + 1):
        for arch in ("x64", "x32"):
            artifacts = args.artifacts_dir / f"dbgeng-{arch}-run-{iteration}"
            run(
                [python, "src/tests/run.py", *live_tests, "--arch", arch,
                 "--engine", "DbgEng", "--timeout", str(args.timeout),
                 "--artifacts-dir", str(artifacts)],
                root,
            )

    if not args.skip_cross_engine:
        smoke = ["script_run_exit/memory", "script_run_exit/multi-session"]
        for arch in ("x64", "x32"):
            for engine in ("TitanEngine", "GleeBug"):
                run(
                    [python, "src/tests/run.py", *smoke, "--arch", arch,
                     "--engine", engine, "--timeout", str(args.timeout),
                     "--artifacts-dir", str(args.artifacts_dir / f"{engine}-{arch}")],
                    root,
                )

    print("DbgEng live gate passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
