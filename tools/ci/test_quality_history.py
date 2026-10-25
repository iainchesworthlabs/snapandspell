# SPDX-License-Identifier: GPL-3.0-or-later
"""Tests for quality_history.py. They need coverage_gate.py from icl-common-infra: set ICL_INFRA to a
checkout (CI uses .icl-infra); without one the tests are skipped."""

import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import quality_history as qh  # noqa: E402

INFRA = Path(os.environ.get("ICL_INFRA", ".icl-infra"))
GATE_FILE = INFRA / "scripts" / "coverage_gate.py"


def line(count, branches=()):
    return {"line_number": 1, "count": count, "branches": [{"count": c} for c in branches]}


REPORT = {
    "files": [
        {"file": "libs/core/a.cpp", "lines": [line(1), line(1), line(0), line(3, (1, 0))]},
        {"file": "libs/scan/b.cpp", "lines": [line(2), line(2, (1, 1))]},
        {"file": "apps/cli/c.cpp", "lines": [{"line_number": 1, "count": 0, "gcovr/noncode": True}]},
    ]
}
FLOORS = {"default": {"line": 90, "branch": 80}, "components": {"libs/core": {}, "libs/scan": {}, "apps/cli": {}}}


class ColourTest(unittest.TestCase):
    def test_the_colour_follows_the_figure(self):
        self.assertEqual(qh.colour(100.0), "brightgreen")
        self.assertEqual(qh.colour(90.0), "brightgreen")
        self.assertEqual(qh.colour(89.9), "green")
        self.assertEqual(qh.colour(75.0), "yellowgreen")
        self.assertEqual(qh.colour(61.0), "yellow")
        self.assertEqual(qh.colour(50.0), "orange")
        self.assertEqual(qh.colour(12.0), "red")

    def test_a_badge_is_a_shields_endpoint(self):
        self.assertEqual(
            qh.badge("coverage", 93.456),
            {"schemaVersion": 1, "label": "coverage", "message": "93.5%", "color": "brightgreen"},
        )
        self.assertEqual(qh.badge("coverage", None)["message"], "n/a")


class HistoryTest(unittest.TestCase):
    def test_a_row_is_appended_and_a_second_run_of_a_commit_replaces_it(self):
        first = qh.update_history("", {"sha": "a", "line": 1})
        second = qh.update_history(first, {"sha": "b", "line": 2})
        again = qh.update_history(second, {"sha": "a", "line": 3})
        self.assertEqual([json.loads(x)["sha"] for x in again.splitlines()], ["b", "a"])
        self.assertEqual(json.loads(again.splitlines()[-1])["line"], 3)

    def test_the_oldest_rows_are_dropped_at_the_limit(self):
        text = ""
        for i in range(7):
            text = qh.update_history(text, {"sha": str(i)}, limit=5)
        self.assertEqual([json.loads(x)["sha"] for x in text.splitlines()], ["2", "3", "4", "5", "6"])

    def test_a_damaged_line_is_dropped_not_fatal(self):
        text = qh.update_history('{"sha": "a"}\nnot json\n', {"sha": "b"})
        self.assertEqual([json.loads(x)["sha"] for x in text.splitlines()], ["a", "b"])


@unittest.skipUnless(GATE_FILE.exists(), "icl-common-infra is not checked out (set ICL_INFRA)")
class FiguresTest(unittest.TestCase):
    def setUp(self):
        self.gate = qh.load_gate(str(GATE_FILE))

    def test_the_figures_are_the_gates(self):
        figures = qh.summarise(self.gate, REPORT, FLOORS)
        # core: 3 of 4 lines, 1 of 2 branches; scan: 2 of 2 lines, 2 of 2 branches; cli has no executable line.
        self.assertEqual(figures["components"]["libs/core"], {"line": 75.0, "branch": 50.0})
        self.assertEqual(figures["components"]["libs/scan"], {"line": 100.0, "branch": 100.0})
        self.assertNotIn("apps/cli", figures["components"])
        self.assertEqual(figures["line"], 83.33)
        self.assertEqual(figures["branch"], 75.0)

    def test_the_files_are_written_and_a_second_commit_extends_the_history(self):
        figures = qh.summarise(self.gate, REPORT, FLOORS)
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            written = qh.write_files(out, figures, "aaa", "2026-10-25")
            self.assertEqual(sorted(written), ["README.md", "coverage-branch.json", "coverage-line.json", "coverage.json", "history.jsonl"])
            line_badge = json.loads((out / "coverage-line.json").read_text())
            self.assertEqual((line_badge["message"], line_badge["color"]), ("83.3%", "green"))
            qh.write_files(out, figures, "bbb", "2026-10-26")
            rows = [json.loads(x) for x in (out / "history.jsonl").read_text().splitlines()]
            self.assertEqual([r["sha"] for r in rows], ["aaa", "bbb"])
            self.assertEqual(json.loads((out / "coverage.json").read_text())["sha"], "bbb")

    def test_main_refuses_a_report_with_nothing_in_it(self):
        with tempfile.TemporaryDirectory() as tmp:
            report = Path(tmp) / "r.json"
            floors = Path(tmp) / "f.json"
            report.write_text(json.dumps({"files": []}))
            floors.write_text(json.dumps(FLOORS))
            code = qh.main(["--gate", str(GATE_FILE), "--report", str(report), "--floors", str(floors),
                            "--out", str(Path(tmp) / "out"), "--sha", "x"])
            self.assertEqual(code, 1)
            self.assertFalse((Path(tmp) / "out").exists())


if __name__ == "__main__":
    unittest.main()
