#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Write the files of the `quality-history` branch from a coverage report (docs/ci.md, "Quality history").

    python3 tools/ci/quality_history.py --gate .icl-infra/scripts/coverage_gate.py \\
        --report build/coverage/coverage.json --floors tools/coverage-floors.json \\
        --out <checkout of the quality-history branch> --sha <commit> [--date 2026-10-25]

The figures are the gate's: coverage_gate.py from icl-common-infra measures the gcovr report against the
components of the floors file, so a badge and the gate cannot disagree. The files written into --out:

    coverage-line.json     a shields.io endpoint badge: the statement coverage of all components
    coverage-branch.json   the same for branches
    coverage.json          the latest figures, overall and for each component, with the commit and date
    history.jsonl          one line a commit, newest last, the last HISTORY_LIMIT kept
    README.md              what the branch is (written once)

A badge is `https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/<owner>/<repo>/quality-history/coverage-line.json`.
"""

from __future__ import annotations

import argparse
import datetime
import importlib.util
import json
import sys
from pathlib import Path

HISTORY_LIMIT = 500

README = """# quality-history

Written by the `Update the quality history` job of `.github/workflows/ci.yml` on every push to `develop`;
nobody edits it by hand. It holds the figures behind the README badges and their trend:

- `coverage-line.json`, `coverage-branch.json`: shields.io endpoint badges
- `coverage.json`: the latest figures, overall and for each component
- `history.jsonl`: one line a commit

See docs/ci.md, "Quality history", on `develop`.
"""

# The lowest figure of each colour, best first.
COLOURS = ((90.0, "brightgreen"), (80.0, "green"), (70.0, "yellowgreen"), (60.0, "yellow"), (50.0, "orange"))


def load_gate(path: str):
    spec = importlib.util.spec_from_file_location("coverage_gate", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def colour(percent: float) -> str:
    return next((name for floor, name in COLOURS if percent >= floor), "red")


def badge(label: str, percent: float | None) -> dict:
    if percent is None:
        return {"schemaVersion": 1, "label": label, "message": "n/a", "color": "lightgrey"}
    return {"schemaVersion": 1, "label": label, "message": f"{percent:.1f}%", "color": colour(percent)}


def summarise(gate, report: dict, floors: dict) -> dict:
    """Overall and per-component figures, from the gate's own measure()."""
    totals, _ = gate.measure(report, list(floors.get("components", {})))
    components = {}
    covered = [0, 0, 0, 0]
    for name in sorted(totals):
        lc, lt, bc, bt = totals[name]
        if lt == 0:
            continue
        components[name] = {"line": round(gate.pct(lc, lt), 2), "branch": None if bt == 0 else round(gate.pct(bc, bt), 2)}
        covered = [a + b for a, b in zip(covered, (lc, lt, bc, bt))]
    lc, lt, bc, bt = covered
    return {
        "line": None if lt == 0 else round(gate.pct(lc, lt), 2),
        "branch": None if bt == 0 else round(gate.pct(bc, bt), 2),
        "components": components,
    }


def update_history(existing: str, row: dict, limit: int = HISTORY_LIMIT) -> str:
    """The history with `row` as its last line; a line for the same commit is replaced, the oldest dropped."""
    lines = [line for line in existing.splitlines() if line.strip()]
    kept = []
    for line in lines:
        try:
            if json.loads(line).get("sha") == row["sha"]:
                continue
        except json.JSONDecodeError:
            continue
        kept.append(line)
    kept.append(json.dumps(row, sort_keys=True, separators=(",", ":")))
    return "\n".join(kept[-limit:]) + "\n"


def write_files(out: Path, figures: dict, sha: str, date: str) -> list[str]:
    out.mkdir(parents=True, exist_ok=True)
    written = []

    def put(name: str, text: str) -> None:
        (out / name).write_text(text, encoding="utf-8", newline="\n")
        written.append(name)

    put("coverage-line.json", json.dumps(badge("coverage (statements)", figures["line"]), indent=2) + "\n")
    put("coverage-branch.json", json.dumps(badge("coverage (branches)", figures["branch"]), indent=2) + "\n")
    latest = {"sha": sha, "date": date, **figures}
    put("coverage.json", json.dumps(latest, indent=2, sort_keys=True) + "\n")
    history = out / "history.jsonl"
    put("history.jsonl", update_history(history.read_text(encoding="utf-8") if history.exists() else "", latest))
    if not (out / "README.md").exists():
        put("README.md", README)
    return written


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", required=True, help="coverage_gate.py from icl-common-infra")
    ap.add_argument("--report", required=True, help="the gcovr JSON report")
    ap.add_argument("--floors", required=True, help="tools/coverage-floors.json: the components")
    ap.add_argument("--out", required=True, type=Path, help="the directory of the quality-history branch")
    ap.add_argument("--sha", required=True)
    ap.add_argument("--date", default=datetime.date.today().isoformat())
    args = ap.parse_args(argv)

    gate = load_gate(args.gate)
    report = json.loads(Path(args.report).read_text(encoding="utf-8"))
    floors = json.loads(Path(args.floors).read_text(encoding="utf-8"))
    figures = summarise(gate, report, floors)
    if figures["line"] is None:
        print("::error::the report has no executable line in any component; nothing to publish")
        return 1
    for name in write_files(args.out, figures, args.sha, args.date):
        print(f"wrote {args.out / name}")
    print(f"statements {figures['line']}%, branches {figures['branch']}%")
    return 0


if __name__ == "__main__":
    sys.exit(main())
