# Branch protection, rulesets and the merge queue

The repository settings, the rulesets and the labels are applied from
`icl-common-infra` with `admin/apply.sh`, so every ICL repository is set up the
same way. This repository carries only what is its own, in
`.github/repo-settings.json` (the description and the topics). The rulesets
are the ones in `icl-common-infra/admin/rulesets/`. This page records what they
say and why.

```sh
# From a checkout of icl-common-infra. A dry run unless --apply is given.
RELEASE_BOT_APP_ID=<id> OWNER_USER=iainchesworth \
  admin/apply.sh --repo iainchesworthlabs/wordjewelssolver \
  --config-dir <this checkout>/.github [--only settings,rulesets,labels] [--apply]
```

Changing a rule means changing it in `icl-common-infra` and applying it again.
Only the owner applies settings.

## The flow

```text
feature/* bugfix/* docs/* chore/* agent/*  ──PR──▶  merge queue  ──▶  develop
                                                                        │
                                       a v* tag on a develop commit ────┘
                                       release workflow ──fast-forward──▶ main
```

- `develop` is the trunk and the default branch. A pull request targets it and
  goes through the merge queue. Nobody pushes to it.
- `main` holds published releases. The release workflow moves it to the tagged
  commit once the release job succeeds, so `main` is always an ancestor of
  `develop`. Nobody opens a pull request into `main`.
- Pull requests are squash-merged. The queue builds each entry on the merged
  tree.
- A patch to an old release uses a `release/x.y` branch cut from its tag.
- Head branches are deleted when their pull request merges.

## The rulesets

| Ruleset | Applies to | What it does |
| --- | --- | --- |
| `develop-protection` | `develop` | A pull request is required (no approvals, since there is one maintainer; stale approvals dismissed; conversations resolved; squash only). Required checks: `CI Status` and `Scan dependency diff`. Linear history. No deletion or force push. No bypass. |
| `merge-queue-develop` | `develop` | The merge queue: squash, `ALLGREEN`, entries built in parallel. |
| `main-release-only` | `main` | Only the release bot can update it. No deletion or force push. |
| `tags` | `v*` tags | A tag cannot be deleted, moved or created again. Creation is for the owner and the release bot. |
| `branch-names` | every other branch | Creation is refused unless the name is one of those below. |

### Required checks

`CI Status` is the job at the end of `ci.yml`. It waits for the policy job,
the vendored-modules check, `clang-format`, `platform-isolation`, the plan, the
build-and-test legs and coverage, and it fails if any of them failed, was
cancelled, or was skipped in a run that should have run it. Which jobs a change
needs is decided in `ci.yml` and `.github/ci/plan.json`; the rulesets do not
name them, so a leg can be added or renamed without touching the rulesets.

`Scan dependency diff` is the dependency review job.

Every workflow that produces a required check also triggers on `merge_group`.
GitHub runs only the workflows that opt in on the queue's temporary branch, and
a required check that never reports holds the entry until it times out. A
workflow that produces no required check does not carry `merge_group`, or each
queue entry would pay for a run of it for nothing.

## Branch names

Allowed: `develop`, `main`, `quality-history`, `gh-pages`, `release/x.y`,
`feature/`, `bugfix/`, `docs/`, `chore/` and `agent/` followed by lower-case
words joined by `-` (up to three levels), `dependabot/...`, and the queue's
`gh-readonly-queue/...`. The same list is applied earlier and more softly by the
`pre-push` hook in `tools/hooks` and by the Policy job on the pull request.

Things learned while the first version of this rule was written:

- **GitHub's name-pattern rule is created and then not enforced** on this plan
  (checked on 2026-10-15, with `regex` and with `starts_with`). The list is
  written as include and exclude globs on a creation rule instead, which is
  enforced.
- **`*` stands for one path segment and `**` does too.** A glob such as
  `gh-readonly-queue/**` does not match the two-segment name the queue creates,
  so the queue could not create its branch and removed every entry within
  seconds. Each depth is listed.
- **The queue's prefix is excluded on purpose.** Without it no pull request can
  merge.
- **GitHub's Revert button** creates a `revert-<n>-...` branch, which is
  refused. Revert with `git revert` on a branch with an allowed name.
- A ref name cannot have a component that starts with a dot, so `.github` never
  appears in a branch name.

## Repository settings

Applied by `admin/apply.sh`: squash merge only, with the pull request title and
body as the commit message; auto-merge on; head branches deleted; no wiki or
projects; workflow permissions read-only, with SHA pinning required for actions;
Dependabot security updates, secret scanning with push protection, private
vulnerability reporting; Pages built from a workflow; a `release` environment
with the owner as the required reviewer. The labels are those in
`icl-common-infra/admin/labels.json`.

## Dependabot pull requests

Dependabot opens its pull requests against `develop` from `dependabot/*`
branches. They go through the same checks and the same queue. The workflow
`dependabot-auto-merge.yml` turns on auto-merge for updates that are not major.

## Other scanners

`scorecard.yml` publishes a score and a badge from `main`. CodeQL's results and
the nightly MSVC `/analyze` results (category `msvc-analyze`) are under
Security, Code scanning. None of them is a required check.
