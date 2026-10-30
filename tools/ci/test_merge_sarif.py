# SPDX-License-Identifier: GPL-3.0-or-later
"""Tests for merge_sarif.py."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import merge_sarif  # noqa: E402


def log(rules, results, artifacts):
    return {
        "version": "2.1.0",
        "runs": [{"tool": {"driver": {"name": "MSVC", "rules": rules}}, "results": results, "artifacts": artifacts}],
    }


def result(rule_id, rule_index, uri, artifact_index):
    return {
        "ruleId": rule_id,
        "ruleIndex": rule_index,
        "locations": [{"physicalLocation": {"artifactLocation": {"uri": uri, "index": artifact_index}}}],
    }


def artifact(uri):
    return {"location": {"uri": uri}}


A = log([{"id": "C6001"}, {"id": "C6011"}], [result("C6011", 1, "a.cpp", 0)], [artifact("a.cpp")])
B = log([{"id": "C6011"}, {"id": "C6385"}], [result("C6385", 1, "b.cpp", 0), result("C6011", 0, "a.cpp", 1)],
        [artifact("b.cpp"), artifact("a.cpp")])


class MergeTest(unittest.TestCase):
    def test_nothing_to_merge(self):
        self.assertIsNone(merge_sarif.merge([]))
        self.assertIsNone(merge_sarif.merge([{"runs": []}]))

    def test_one_run_with_every_result(self):
        merged = merge_sarif.merge([A, B])
        self.assertEqual(len(merged["runs"]), 1)
        self.assertEqual([r["ruleId"] for r in merged["runs"][0]["results"]], ["C6011", "C6385", "C6011"])

    def test_each_rule_is_listed_once_and_results_point_at_it(self):
        run = merge_sarif.merge([A, B])["runs"][0]
        rules = [rule["id"] for rule in run["tool"]["driver"]["rules"]]
        self.assertEqual(sorted(rules), ["C6011", "C6385"])
        for found in run["results"]:
            self.assertEqual(rules[found["ruleIndex"]], found["ruleId"])

    def test_each_artifact_is_listed_once_and_results_point_at_it(self):
        run = merge_sarif.merge([A, B])["runs"][0]
        uris = [a["location"]["uri"] for a in run["artifacts"]]
        self.assertEqual(sorted(uris), ["a.cpp", "b.cpp"])
        for found in run["results"]:
            location = found["locations"][0]["physicalLocation"]["artifactLocation"]
            self.assertEqual(uris[location["index"]], location["uri"])

    def test_the_inputs_are_left_alone(self):
        before = json.dumps([A, B])
        merge_sarif.merge([A, B])
        self.assertEqual(json.dumps([A, B]), before)

    def test_a_directory_of_logs_becomes_one_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "sub").mkdir()
            (root / "a.sarif").write_text(json.dumps(A), encoding="utf-8")
            (root / "sub" / "b.sarif").write_text(json.dumps(B), encoding="utf-8")
            out = root / "out" / "merged.sarif"
            self.assertEqual(merge_sarif.main(["merge_sarif.py", str(root), str(out)]), 0)
            self.assertEqual(len(json.loads(out.read_text(encoding="utf-8"))["runs"]), 1)

    def test_the_output_is_not_read_back_in(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "a.sarif").write_text(json.dumps(A), encoding="utf-8")
            out = root / "merged.sarif"
            merge_sarif.main(["merge_sarif.py", str(root), str(out)])
            merge_sarif.main(["merge_sarif.py", str(root), str(out)])
            self.assertEqual(len(json.loads(out.read_text(encoding="utf-8"))["runs"][0]["results"]), 1)


if __name__ == "__main__":
    unittest.main()
