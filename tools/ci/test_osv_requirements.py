# SPDX-License-Identifier: GPL-3.0-or-later
"""osv-scanner.yml names every pinned requirements file, and only those.

OSV-Scanner recognises a file called requirements.txt and no other, so each file of
.github/requirements/ is passed to it with --lockfile=requirements.txt:<path>. A file added there and not
named in the workflow would be pinned and never scanned; a name with no file would fail the scan.
"""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / ".github" / "workflows" / "osv-scanner.yml"
REQUIREMENTS = ROOT / ".github" / "requirements"


class OsvRequirementsTest(unittest.TestCase):
    def named(self):
        text = WORKFLOW.read_text(encoding="utf-8")
        return set(re.findall(r"^\s*--lockfile=requirements\.txt:(\S+)\s*$", text, re.MULTILINE))

    def test_every_requirements_file_is_named_in_the_scan(self):
        on_disk = {p.relative_to(ROOT).as_posix() for p in REQUIREMENTS.glob("*.txt")}
        self.assertTrue(on_disk, "no requirements files found: the test is looking in the wrong place")
        self.assertEqual(self.named(), on_disk)


if __name__ == "__main__":
    unittest.main()
