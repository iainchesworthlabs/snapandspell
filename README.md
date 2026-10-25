# quality-history

Written by the `Update the quality history` job of `.github/workflows/ci.yml` on every push to `develop`;
nobody edits it by hand. It holds the figures behind the README badges and their trend:

- `coverage-line.json`, `coverage-branch.json`: shields.io endpoint badges
- `coverage.json`: the latest figures, overall and for each component
- `history.jsonl`: one line a commit

See docs/ci.md, "Quality history", on `develop`.
