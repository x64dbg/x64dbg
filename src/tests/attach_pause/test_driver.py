"""Deterministic checks for the break-in driver's bounded pause retries."""

import argparse
import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

import driver


class BreakinDriverTests(unittest.TestCase):
    def run_driver(self, responses):
        headless = Mock()
        headless.process.poll.return_value = 0
        headless.wait_for_line.side_effect = responses
        target = Mock()
        target.poll.return_value = 0
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            args = argparse.Namespace(
                headless=str(root / "headless.exe"),
                debuggee=str(root / "attach_pause.exe"),
                userdir=str(root / "userdir"),
                log=str(root / "debug.log"),
                artifacts_dir=str(root),
                script="test.breakin.txt",
                no_console_window=False,
            )
            with (
                patch.object(driver, "parse_args", return_value=args),
                patch.object(driver.subprocess, "Popen", return_value=target),
                patch.object(driver, "wait_for_ready", return_value="ready 10 11"),
                patch.object(driver, "Headless", return_value=headless),
                patch.object(driver, "append_log") as log,
                contextlib.redirect_stdout(io.StringIO()),
            ):
                result = driver.main()
        pauses = sum(call.args == ("pause",) for call in headless.send.call_args_list)
        messages = [call.args[1] for call in log.call_args_list]
        return result, pauses, messages, headless.wait_for_line.call_args_list

    def test_late_event_requires_another_pause_request(self):
        # Late worker creation resets the debugger's no-event interval. The
        # second pause therefore cannot spawn the break-in thread; the third can.
        result, pauses, messages, waits = self.run_driver(
            ["Attached to process!", None, None, "[STATE] paused"]
        )
        self.assertEqual(result, 0)
        self.assertEqual(pauses, 3)
        self.assertIn("[x64dbg-test] FINAL status=pass asserts=2", messages)
        self.assertEqual([call.args[1] for call in waits], [30, 4, 2.5, 2.5])

    def test_retries_are_bounded_and_still_fail_without_a_pause(self):
        result, pauses, messages, waits = self.run_driver(
            ["Attached to process!", None, None, None, None, None]
        )
        self.assertEqual(result, 1)
        self.assertGreaterEqual(pauses, 2)
        self.assertLessEqual(pauses, 5)
        self.assertIn("reason=breakin_no_effect", messages[-1])
        self.assertLessEqual(sum(call.args[1] for call in waits[2:]), 10)

    def test_first_request_must_not_pause(self):
        result, pauses, messages, _ = self.run_driver(
            ["Attached to process!", "[STATE] paused"]
        )
        self.assertEqual(result, 1)
        self.assertEqual(pauses, 1)
        self.assertIn("reason=unexpected_pause", messages[-1])


if __name__ == "__main__":
    unittest.main()
