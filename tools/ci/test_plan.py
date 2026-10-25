# SPDX-License-Identifier: GPL-3.0-or-later
"""Tests for plan.py. They need plan_gate.py from icl-common-infra: set ICL_INFRA to a checkout
(CI uses .icl-infra); without one the tests are skipped."""

import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import plan as planner  # noqa: E402

INFRA = Path(os.environ.get("ICL_INFRA", ".icl-infra"))
GATE_FILE = INFRA / "scripts" / "plan_gate.py"
ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(GATE_FILE.exists(), "icl-common-infra is not checked out (set ICL_INFRA)")
class PlanTest(unittest.TestCase):
    def setUp(self):
        self.gate = planner.load_gate(str(GATE_FILE))
        self.cfg = planner.load_config(self.gate, str(ROOT / ".github" / "ci" / "plan.json"))

    def decide(self, event, paths, draft=False, labels=()):
        return planner.decide(event, draft=draft, labels=list(labels), paths=paths, cfg=self.cfg, gate=self.gate)

    def test_documentation_only_runs_nothing(self):
        for event in ("pull_request", "merge_group", "push"):
            plan = self.decide(event, ["docs/ci.md", "changelog.d/x.fixed.md", "README.md"])
            self.assertEqual(plan["legs"], [])
            self.assertFalse(any(plan[key] for key in planner.OTHER))

    def test_a_change_no_build_reads_runs_nothing(self):
        self.assertEqual(self.decide("pull_request", [".github/CODEOWNERS"])["legs"], [])

    def test_pull_request_builds_linux_and_coverage(self):
        plan = self.decide("pull_request", ["libs/core/src/solver.cpp"])
        self.assertEqual(plan["legs"], ["linux"])
        self.assertTrue(plan["coverage"] and plan["fuzz"])
        self.assertFalse(plan["tsan"] or plan["performance"] or plan["package"])

    def test_draft_runs_nothing(self):
        plan = self.decide("pull_request", ["libs/core/src/solver.cpp"], draft=True)
        self.assertEqual(plan["legs"], [])
        self.assertFalse(plan["coverage"] or plan["fuzz"])

    def test_deep_label_runs_everything_even_on_a_draft(self):
        plan = self.decide("pull_request", ["docs/ci.md"], draft=True, labels=["deep"])
        self.assertEqual(plan["legs"], planner.DESKTOP)
        self.assertTrue(plan["tsan"] and plan["performance"] and plan["package"] and plan["coverage"] and plan["fuzz"])

    def test_queue_adds_windows_but_not_macos(self):
        plan = self.decide("merge_group", ["libs/core/src/solver.cpp"])
        self.assertEqual(plan["legs"], ["linux", "windows"])
        self.assertTrue(plan["tsan"] and plan["performance"] and plan["package"] and plan["coverage"] and plan["fuzz"])

    def test_queue_adds_macos_for_platform_code_cmake_and_the_gate(self):
        for path in ("libs/platforms/macos/foo.cpp", "cmake/Warnings.cmake", "CMakeLists.txt", ".github/workflows/ci.yml"):
            self.assertEqual(self.decide("merge_group", [path])["legs"], ["linux", "windows", "macos"], path)

    def test_queue_with_an_unreadable_diff_includes_macos(self):
        self.assertEqual(self.decide("merge_group", None)["legs"], ["linux", "windows", "macos"])

    def test_push_runs_everything_for_a_build_change(self):
        self.assertEqual(self.decide("push", ["libs/scan/src/a.cpp"])["legs"], planner.DESKTOP)

    def test_a_path_nobody_listed_builds_everything(self):
        self.assertEqual(self.decide("pull_request", ["surprise/file.bin"])["legs"], ["linux"])
        self.assertEqual(self.decide("push", ["surprise/file.bin"])["legs"], planner.DESKTOP)

    def test_an_unreadable_diff_builds_everything(self):
        self.assertEqual(self.decide("push", None)["legs"], planner.DESKTOP)

    def test_dispatch_all(self):
        plan = planner.dispatch("all")
        self.assertEqual(plan["legs"], planner.DESKTOP)
        self.assertTrue(plan["tsan"] and plan["performance"] and plan["package"] and plan["coverage"] and plan["fuzz"])

    def test_dispatch_names_and_profiling(self):
        plan = planner.dispatch("macos,tsan")
        self.assertEqual(plan["legs"], ["macos"])
        self.assertTrue(plan["tsan"])
        self.assertFalse(plan["performance"] or plan["package"] or plan["coverage"] or plan["fuzz"])
        self.assertTrue(planner.dispatch("fuzz")["fuzz"])
        self.assertEqual(planner.dispatch("profiling")["legs"], planner.PROFILING)
        self.assertEqual(planner.dispatch("profiling-linux")["legs"], ["profiling-linux"])
        self.assertEqual(planner.dispatch("all")["legs"], planner.DESKTOP)

    def test_dispatch_rejects_unknown_and_spaced_names(self):
        for legs in ("windos", "linux, macos"):
            with self.assertRaises(ValueError):
                planner.dispatch(legs)

    def test_expected_matches_the_plan(self):
        plan = self.decide("merge_group", ["libs/core/src/solver.cpp"])
        self.assertEqual(
            planner.expected(plan),
            {
                "build-and-test": True,
                "performance": True,
                "package-windows": True,
                "coverage": True,
                "tsan": True,
                "fuzz-regress": True,
            },
        )
        self.assertFalse(any(planner.expected(self.decide("pull_request", ["docs/a.md"])).values()))


if __name__ == "__main__":
    unittest.main()
