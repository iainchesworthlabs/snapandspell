#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the pull request gate's static checks on this checkout before a push.

    python3 tools/ci/precheck.py [--base origin/develop]

The checks are the shared ones from icl-common-infra (branch name, commit text, preprocessor
policy, action pinning, changelog fragment, REUSE, clang-format), plus what the gate will plan
for the diff. This script finds that repository: $ICL_INFRA, or ../icl-common-infra, or
.icl-infra (where CI checks it out). Commit first: uncommitted work is not read.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def find_infra() -> Path | None:
    candidates = [os.environ.get("ICL_INFRA", ""), str(ROOT.parent / "icl-common-infra"), str(ROOT / ".icl-infra")]
    for candidate in candidates:
        if candidate and (Path(candidate) / "scripts" / "precheck.py").is_file():
            return Path(candidate)
    return None


def main() -> int:
    infra = find_infra()
    if infra is None:
        print("precheck: icl-common-infra not found. Clone it next to this repository, or set ICL_INFRA.",
              file=sys.stderr)
        return 2
    command = [sys.executable, str(infra / "scripts" / "precheck.py"), "--root", str(ROOT), *sys.argv[1:]]
    return subprocess.run(command, check=False).returncode


if __name__ == "__main__":
    sys.exit(main())
