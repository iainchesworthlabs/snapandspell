# SPDX-License-Identifier: GPL-3.0-or-later
"""Merge the SARIF logs MSVC's analyzer writes, one per translation unit, into one log with one run.

    merge_sarif.py <directory> <output.sarif>

`github/codeql-action/upload-sarif` refuses a log with several runs that share a category, and the analyzer
writes a log of its own for every source file. The merged run keeps the tool of the first log, every rule
once, every result, and the artifact table the results point into."""

import copy
import json
import sys
from pathlib import Path


def rules_of(run):
    return run.get("tool", {}).get("driver", {}).get("rules", [])


def remap_artifacts(node, old_artifacts, index_of, merged_artifacts):
    """Point every `artifactLocation.index` in `node` at the merged artifact table."""
    if isinstance(node, dict):
        location = node.get("artifactLocation")
        if isinstance(location, dict) and isinstance(location.get("index"), int):
            artifact = old_artifacts[location["index"]]
            uri = artifact.get("location", {}).get("uri", location.get("uri"))
            if uri not in index_of:
                index_of[uri] = len(merged_artifacts)
                merged_artifacts.append(artifact)
            location["index"] = index_of[uri]
        for value in node.values():
            remap_artifacts(value, old_artifacts, index_of, merged_artifacts)
    elif isinstance(node, list):
        for value in node:
            remap_artifacts(value, old_artifacts, index_of, merged_artifacts)


def merge(logs):
    """One SARIF log from several; None when there is no run in any of them."""
    runs = [run for log in logs for run in log.get("runs", [])]
    if not runs:
        return None

    merged = copy.deepcopy(runs[0])
    merged["results"] = []
    merged["artifacts"] = []
    merged.get("tool", {}).get("driver", {})["rules"] = []
    merged_rules = merged["tool"]["driver"]["rules"]
    rule_index = {}
    artifact_index = {}

    for run in runs:
        old_rules = rules_of(run)
        old_artifacts = run.get("artifacts", [])
        for result in copy.deepcopy(run.get("results", [])):
            position = result.get("ruleIndex")
            if isinstance(position, int) and position < len(old_rules):
                rule = old_rules[position]
                key = rule.get("id", result.get("ruleId"))
                if key not in rule_index:
                    rule_index[key] = len(merged_rules)
                    merged_rules.append(rule)
                result["ruleIndex"] = rule_index[key]
            remap_artifacts(result, old_artifacts, artifact_index, merged["artifacts"])
            merged["results"].append(result)

    log = {key: value for key, value in logs[0].items() if key != "runs"}
    log["runs"] = [merged]
    return log


def main(argv):
    if len(argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    source, output = Path(argv[1]), Path(argv[2])
    logs = [json.loads(path.read_text(encoding="utf-8")) for path in sorted(source.rglob("*.sarif"))
            if path.resolve() != output.resolve()]
    log = merge(logs)
    if log is None:
        print(f"no SARIF runs under {source}; nothing written")
        return 0
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(log), encoding="utf-8")
    print(f"{len(logs)} logs, {len(log['runs'][0]['results'])} results, {len(log['runs'][0]['tool']['driver']['rules'])} rules -> {output}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
