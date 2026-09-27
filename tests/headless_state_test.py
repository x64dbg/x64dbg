"""Verify headless state tags and driver handling of delayed GUI state pairs.

Compiles the production GUI_SET_DEBUG_STATE handler and string conversion,
then replays immediate events interleaved with delayed GUI notifications.
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

from trace_filter_test import function

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tests/trace_party"))
from pause_driver import is_live_state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="clang++")
    args = parser.parse_args()
    events = [
        ("initialized", False),
        ("running", True), ("paused", True),  # actual initial stop
        ("running", False), ("paused", False),  # delayed initial pair
        ("running", True),  # the newly requested run
        ("paused", False),  # another stale GUI notification
        ("paused", True),  # actual breakpoint reached
        ("paused", False), ("stopped", False),
    ]
    code = """
#include <cstdio>
#include <cstdint>
using duint = uintptr_t;
enum DBGSTATE { initialized, paused, running, stopped };
const int GUI_SET_DEBUG_STATE = 1;
"""
    code += function("src/headless/tostring.h", "static const char* dbgstate2str(")
    code += """
void emit(DBGSTATE state, bool fast)
{
    auto param1 = (void*)duint(state);
    auto param2 = (void*)duint(fast);
    switch(GUI_SET_DEBUG_STATE)
    {
"""
    code += function("src/headless/headless.cpp", "case GUI_SET_DEBUG_STATE:")
    code += "\nbreak;\n}\n}\nint main()\n{\n"
    code += "\n".join(f"emit({state}, {'true' if fast else 'false'});" for state, fast in events)
    code += "\n}\n"
    with tempfile.TemporaryDirectory(prefix="x64dbg-headless-state-") as directory:
        temp = Path(directory)
        (temp / "test.cpp").write_text(code)
        executable = temp / "test.exe"
        subprocess.run([args.compiler, "-std=c++14", str(temp / "test.cpp"), "-o", str(executable)], check=True)
        output = subprocess.check_output([str(executable)], text=True).splitlines()
    assert [line for line in output if line.startswith("[STATE] ")] == [
        f"[STATE] {state}" for state, _ in events
    ], "legacy state output must remain compatible"
    live = [(index, state) for index, line in enumerate(output)
            for state in ("running", "paused") if is_live_state(line, state)]
    assert [state for _, state in live] == ["running", "paused", "running", "paused"]
    assert all(output[index].startswith("[STATE-FAST] ") for index, _ in live)
    # A second wait must not complete on either the old running/paused pair or
    # the stale pause following the new run. It must reach the actual breakpoint.
    assert live[2][0] > output.index("[STATE] paused", live[1][0] + 2)
    assert live[3][0] > output.index("[STATE] paused", live[2][0])
    print("Headless immediate/delayed state regression tests passed")


if __name__ == "__main__":
    main()
