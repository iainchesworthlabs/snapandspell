#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Decide which jobs of .github/workflows/ci.yml run (docs/ci.md, "What runs when").

    python3 tools/ci/plan.py --gate .icl-infra/scripts/plan_gate.py --config .github/ci/plan.json \\
        --event pull_request --draft false --labels '["deep"]' --base <sha> --head <sha>

The changed paths and the scope (build, gui, docs only) come from plan_gate.py in
icl-common-infra. This script turns the scope and the event into the jobs to run:

    pull request   Linux GCC, coverage and the fuzz regression replay, when the change can alter the build
                   or the tests
    merge queue    the same, plus Windows, performance, package and TSan; macOS when the change
                   touches platform code, CMake or the gate itself
    push           every leg
    draft          nothing but the static checks (a `deep` label overrides)
    deep label     every leg, whatever the change
    dispatch       the legs named in `--legs`; the profiling legs only by name

A diff that cannot be read, an empty diff and an unrecognised path all mean "build everything".

It prints key=value lines for $GITHUB_OUTPUT: legs (a JSON list for the build-and-test matrix),
performance, package, coverage, tsan, fuzz (true or false) and expected (job id to whether the plan
asked for it, which the CI Status job holds the run to).
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import subprocess
import sys
from pathlib import Path

DESKTOP = ["linux", "windows", "macos"]
PROFILING = ["profiling-windows", "profiling-linux", "profiling-macos"]
OTHER = ["performance", "package", "coverage", "tsan", "fuzz"]
DISPATCH_NAMES = (
    ["all", "clang-format", "platform-isolation", "windows", "linux", "macos", "coverage", "perf", "package", "tsan", "fuzz"]
    + ["profiling"]
    + PROFILING
)
# The job ids of ci.yml that the plan decides.
JOB_IDS = {
    "performance": "performance",
    "package": "package-windows",
    "coverage": "coverage",
    "tsan": "tsan",
    "fuzz": "fuzz-regress",
}


def load_gate(path: str):
    spec = importlib.util.spec_from_file_location("plan_gate", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_config(gate, path: str | None) -> dict:
    cfg = dict(gate.DEFAULT)
    if path and Path(path).exists():
        cfg.update(json.loads(Path(path).read_text(encoding="utf-8")))
    return cfg


def changed_paths(base: str, head: str) -> list[str] | None:
    """The paths that differ between base and head, or None when git cannot say."""
    if not base or not head or set(base) == {"0"}:
        return None
    result = subprocess.run(["git", "diff", "--name-only", f"{base}...{head}"], capture_output=True, text=True)
    if result.returncode != 0:
        return None
    return result.stdout.splitlines()


def empty() -> dict:
    return {
        "legs": [],
        "performance": False,
        "package": False,
        "coverage": False,
        "tsan": False,
        "fuzz": False,
        "reason": "",
    }


def everything(reason: str) -> dict:
    plan = empty()
    plan.update(legs=list(DESKTOP), performance=True, package=True, coverage=True, tsan=True, fuzz=True, reason=reason)
    return plan


def dispatch(legs: str) -> dict:
    """A manual run: the legs it names, as ci.yml always read them."""
    names = [name for name in legs.split(",") if name] or ["all"]
    unknown = [name for name in names if name not in DISPATCH_NAMES]
    if unknown or any(char.isspace() for char in legs):
        raise ValueError(
            f"unknown or malformed legs '{legs}' (names: {', '.join(DISPATCH_NAMES)}; comma-separated, no spaces)"
        )
    wanted = set(names)
    everything_named = "all" in wanted
    plan = empty()
    plan["legs"] = [leg for leg in DESKTOP if everything_named or leg in wanted]
    plan["legs"] += [leg for leg in PROFILING if "profiling" in wanted or leg in wanted]
    plan["performance"] = everything_named or "perf" in wanted
    plan["package"] = everything_named or "package" in wanted
    plan["coverage"] = everything_named or "coverage" in wanted
    plan["tsan"] = everything_named or "tsan" in wanted
    plan["fuzz"] = everything_named or "fuzz" in wanted
    plan["reason"] = f"manual run: {legs or 'all'}"
    return plan


def decide(event: str, *, draft: bool, labels: list[str], paths: list[str] | None, cfg: dict, gate) -> dict:
    deep = "deep" in labels
    scope = gate.plan(paths or [], cfg)
    if deep:
        return everything("the deep label")
    if event == "pull_request" and draft:
        plan = empty()
        plan["reason"] = "draft pull request: static checks only"
        return plan
    if scope["docs_only"] == "true":
        plan = empty()
        plan["reason"] = "documentation only: static checks only"
        return plan
    if scope["build"] != "true":
        plan = empty()
        plan["reason"] = "nothing a build reads changed: static checks only"
        return plan

    reason = scope["reason"] or "a build path changed"
    if event == "pull_request":
        plan = empty()
        plan.update(legs=["linux"], coverage=True, fuzz=True, reason=reason)
        return plan
    if event == "merge_group":
        platform = (
            not paths  # a diff nobody could read: assume the worst
            or scope["machinery"] == "true"
            or any(gate.match(p, cfg.get("platform", [])) for p in paths)
        )
        plan = everything(reason)
        plan["legs"] = ["linux", "windows"] + (["macos"] if platform else [])
        return plan
    return everything(reason)


def expected(plan: dict) -> dict:
    jobs = {"build-and-test": bool(plan["legs"])}
    jobs.update({job: bool(plan[key]) for key, job in JOB_IDS.items()})
    return jobs


def outputs(plan: dict) -> dict:
    out = {"legs": json.dumps(plan["legs"]), "expected": json.dumps(expected(plan))}
    out.update({key: str(plan[key]).lower() for key in OTHER})
    return out


def summary(event: str, plan: dict) -> str:
    rows = [("Build and test legs", ", ".join(plan["legs"]) or "none")]
    rows += [(key.capitalize(), "yes" if plan[key] else "no") for key in OTHER]
    lines = [f"### Plan ({event})", "", f"Reason: {plan['reason']}", "", "| Job | Runs |", "|---|---|"]
    return "\n".join(lines + [f"| {name} | {value} |" for name, value in rows]) + "\n"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", required=True, help="plan_gate.py from icl-common-infra")
    ap.add_argument("--config", default=".github/ci/plan.json")
    ap.add_argument("--event", required=True)
    ap.add_argument("--draft", default="false")
    ap.add_argument("--labels", default="[]", help="JSON list of label names")
    ap.add_argument("--base", default="")
    ap.add_argument("--head", default="")
    ap.add_argument("--legs", default="all", help="manual runs: comma-separated leg names")
    ap.add_argument("--github-output")
    ap.add_argument("--summary")
    args = ap.parse_args(argv)

    gate = load_gate(args.gate)
    cfg = load_config(gate, args.config)
    try:
        if args.event == "workflow_dispatch":
            plan = dispatch(args.legs)
        else:
            plan = decide(
                args.event,
                draft=args.draft == "true",
                labels=json.loads(args.labels or "[]"),
                paths=changed_paths(args.base, args.head),
                cfg=cfg,
                gate=gate,
            )
    except ValueError as error:
        print(f"::error::{error}")
        return 1

    for key, value in outputs(plan).items():
        print(f"{key}={value}")
    text = summary(args.event, plan)
    if args.github_output:
        with open(args.github_output, "a", encoding="utf-8") as handle:
            handle.writelines(f"{key}={value}\n" for key, value in outputs(plan).items())
    if args.summary:
        with open(args.summary, "a", encoding="utf-8") as handle:
            handle.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
