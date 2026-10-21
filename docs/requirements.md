# Requirements and traceability

What the product is required to do, where each requirement is built, which test
shows it, and what is not yet shown. The requirements are in the
[product requirements](product-requirements.md) and the
[design pages](design/index.md); they carry no numbers of their own, so this
section gives each a stable ID and one row. It replaces the work-plan audit that
was kept in `docs/audit-0.1.md` until 2026-10-22 (git history keeps it).

**Verified at 2026-10-22, commit b1aeb44 (origin/develop).** Every status below
was read from the code and the test files at that commit by opening them. Nothing
was run for this page except the check that keeps it honest
([below](#the-check)); a test cited here passes in CI or it does not, and this
page does not say which. The pages carry the same line, and the check fails a
page without one.

## The pages

| Area | Rows | Implemented | Partial | Decided | Deferred | Not built | Doc only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| [Board, dice and shake](requirements/board.md) | 62 | 39 | 13 | 4 | 3 | 2 | 1 |
| [Game, tracing and progress](requirements/game.md) | 57 | 32 | 12 | 8 | 1 | 3 | 1 |
| [Phonics and the word library](requirements/phonics.md) | 59 | 34 | 12 | 2 | 5 | 3 | 3 |
| [Scanner and Mode A](requirements/scanner.md) | 56 | 35 | 14 | 1 | 4 | 2 | 0 |
| [Parent area, dashboard and privacy](requirements/parent.md) | 74 | 47 | 11 | 6 | 5 | 3 | 2 |
| [Application, audio, languages and platforms](requirements/application.md) | 78 | 34 | 32 | 4 | 3 | 5 | 0 |
| [Performance, constraints and licensing](requirements/performance.md) | 67 | 29 | 25 | 4 | 5 | 2 | 2 |
| **All** | **453** | **250** | **119** | **29** | **26** | **20** | **9** |

Rows with no test: **31** (Implemented or Partial rows whose "Demonstrated by"
cell is empty). The check holds this number to the pages: it may fall only by a
test being cited, and rise never.

## Reading a row

| Column | Meaning |
| --- | --- |
| **ID** | `R-<AREA>-<n>`, optionally a lower-case letter where a row was split. Never reused or renumbered: a requirement that is dropped keeps its ID with the status Decided or Deferred. A row may carry several IDs where they share a status and evidence. |
| **Requirement** | One sentence, with the document that states it in brackets. |
| **Status** | **Implemented**: the code does it and a test or a recorded check shows it. **Partial**: some of it, or it is built and not shown (the note says which). **Not built**: the documents ask for it and the code does not do it. **Deferred**: after 0.1 on purpose, or blocked on something outside the code (the literacy review, a device, an account). **Decided**: built or dropped by a recorded decision that differs from the original text ([the decisions](#decisions)). **Doc only**: not testable in code (a repository fact, a review). |
| **Implemented in** | The files, without line numbers: they go stale. |
| **Demonstrated by** | `` `path::name` `` references: a Catch2 case name, a QML `test_` function, or the ctest name in the CMake file that registers it. `—` means nothing demonstrates it. |
| **Notes** | What a reader needs to trust the row: skipped in some legs, only on Windows, tested against a stub, a document that disagrees. |

**What "Implemented" does not mean.** It never means "works on a real board, a real
camera or a real device": no real photograph has been read by the scanner, no
accelerometer, haptic engine or screen reader has been tried, and the iOS build has
never run. Several tests are conditional, and the note says so: the performance
work budgets skip in a Debug build (all three required CI legs are Debug, so the
budgets run only in the non-required Release leg and in `release.yml`); the
checks that need PowerShell, the Linguist tools, Tracy or a display skip where they
are missing; and the QML suites run over a stub scene and a fake game, not the real
`Main.qml` shell.

## The check

`requirements.matrix` (a ctest in the `quality` tier, `tests/requirements/check_requirements.cmake`,
text only, so it runs in every CI leg) fails when:

- a page has no `Verified at <date>, commit <sha>` line;
- a row's ID is not of the form `R-<AREA>-<n>`, appears twice, or its status is not
  one of the six words;
- a `` `path::name` `` reference names a file that does not exist, or a name that is
  not in that file as written, so a renamed or deleted test breaks the row that
  cites it;
- a path in "Implemented in" does not exist;
- the "Rows with no test" figure above is not the number the pages have.

`requirements.matrix.selftest` holds the check to fourteen cases on throw-away trees.
The check proves a cited test **exists**, not that it passes or that it proves the
row: that is what the status and the notes are for, and what a reviewer of a change
to a row is for. When you rename a test, `ctest -R requirements.matrix` names the rows
to correct; when you change a design number or a default, change the row in the same
commit.

## Decisions

Decisions that changed a requirement's text, with the date they were taken. The
roadmap's [decision table](roadmap.md#4-decisions-that-gate-the-plan) holds them
with the pages they amended (D8 to D12).

| Date | Decision | Rows |
| --- | --- | --- |
| 2026-10-21 | A **board clock** the parent sets (count up or down, off by default) stays; PRD 12.3 "no timers or limits" is amended | [R-GAM-17](requirements/game.md), [R-PAR-30](requirements/parent.md), R-APP-33 |
| 2026-10-21 | The **parental gate is one two-second press-and-hold**, not two fingers on two spots | [R-PAR-01](requirements/parent.md) to R-PAR-03, R-PAR-09 |
| 2026-10-21 | **My jewels** (best board for each size, points, the week) is shown to the child without the gate; "shown to the parent only" is amended | R-DSH-14, R-DSH-18 |
| 2026-10-16, recorded 2026-10-21 | The **standard board plays the full ENABLE list with no block list**; "no unsuitable word is ever shown" holds for the phonics board only, and a test pins the risk | R-LIB-13, R-LIB-16, R-GAM-08b |
| 2026-10-21 | 0.1 is the three **desktops** plus an **ad-hoc iOS** build for the author's devices; **Android** is in progress and not built | R-PLT-02 to R-PLT-06 |
| 2026-10-21, to confirm | The **three-step help**, the **resting screen not built** and **Back leaving the round open** are recorded from the build; the author has not yet confirmed them | R-GAM-14, R-GAM-15, R-GAM-18, R-GAM-23 |

## The ID convention, and what is not built

Built: the IDs above, the pages, and the check. The ID of a requirement is its
row; the row cites its tests, not the other way round.

Proposed, **not built** (the cost is a pass over about 1,000 tests): tag each Catch2
case with its ID (`[R-GAM-07]`; the unit tier's `catch_discover_tests` leaves tags out
of the ctest labels on purpose, so a tool would read `--list-tests`), put a
`// req: R-APP-13` comment above each QML `test_` function and a `# req:` header
on each script check, so that a test can be found from its requirement and a test
that cites an ID that is not a row can fail the check. Also proposed: join each
required CI leg's JUnit report with the rows and fail when an Implemented row's only
passing evidence is skipped in every required leg (this would have caught the
performance budgets, which no required leg runs); generate the status tables from a
data file instead of editing markdown; and require, by `CODEOWNERS` and a line in the
pull request template, that a change to a default or a design number touches its row.

## Gaps that matter most

The pages end with their own "Remaining gaps", each naming the test that would close
it and where it would live. The ones that stand between 0.1 and a release you can
trust:

- **No real frame has been read** and no camera or device has been used
  ([Scanner](requirements/scanner.md)); the scanner's accuracy is a regression gate on
  drawn frames. A recorded harness exists, is tested, and has never run on photographs.
- **No required check holds a performance budget**
  ([Performance](requirements/performance.md)): they skip in Debug, and the one
  Release leg is not a required context. The release workflow runs them on a tag.
- **There is no phonics content** and no literacy reviewer
  ([Phonics](requirements/phonics.md)), so the shipped game is the standard board; the
  sound-it-out machinery is built and has no clips.
- **The release pipeline has not run on GitHub**, and `main` is behind `develop`, so
  scheduled and dispatched workflows cannot start
  ([Performance](requirements/performance.md), the gates).
- **Accessibility is by convention**: no test reads an accessible name or role, the
  found-word announcement is not built, and nothing has been tried with a screen
  reader ([Application](requirements/application.md)).
- **The Main.qml shell and the route into Mode A are tested only through stubs**
  ([Application](requirements/application.md), [Scanner](requirements/scanner.md)).
- **Android and the web do not exist; iOS has never run** (R-PLT-04 to R-PLT-06).
- **The gate is a brake and the board has no block list**: both by decision, both
  with the weakness stated.
