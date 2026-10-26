# Security and threat model

Reports of a vulnerability go through the process in
[SECURITY.md](https://github.com/iainchesworthlabs/wordjewelssolver/blob/develop/SECURITY.md). This page
says what the project protects, from whom, and what keeps it true.

## What is protected

The program is for children of five and six and their grown-ups. It keeps a small local file of what the
child has found, and nothing else about them. That file, the child's trust in the program and the
integrity of what is downloaded are the assets.

## Rules that follow from it

| Rule | Kept by |
| --- | --- |
| No audio recording and no picture capture or storage | the camera session is video only and frames are never written; `privacy.sources` scans first-party code |
| No network capability and no use of the network | `privacy.sources` scans for the network classes and headers, URL literals and the Qt modules whose job is the network |
| No export: no share, print, save-through-a-dialog, clipboard or mail | the same scan |
| A local store with only what the dashboard needs (no duration of play, identity, location, image or sound) | the store's own tests and the `[privacy]` cases |
| No profiling in what ships | the Release presets turn Tracy off; `cpack` refuses to package a Tracy build; `check_package.ps1` rejects `TracyClient`, a sanitizer runtime and `.pdb` files |

The details and what is not yet checked (a built Android package's permissions, an iOS property list) are
in [Application design](design/application.md#privacy-built-in-and-checked) and
[The parent area as built](design/parent-area.md#privacy-checks-ci).

## Untrusted input

The program reads files that a person or another program may have written: the board text, the store
file, the phonics phase files, the word list and the dice file. The store is versioned, written
atomically and capped in size, and a damaged or newer file is refused without overwriting the good one.
Each of these readers, and the scanner's reading of a camera frame, has a libFuzzer harness
([Fuzzing](fuzzing.md)); the pull request gate replays the seeds and the inputs that once crashed one, and
`fuzz.yml` fuzzes on a push to `develop` and every night. The command line's argument parser has none yet.

## Hardening

The libraries, the command line and the application are built with `icl::hardening` from
icl-common-infra (`cmake/Hardening.cmake`, option `SNS_ENABLE_HARDENING`, on in every preset but the fuzzing one). With GCC and
Clang that is `-fstack-protector-strong`, `_FORTIFY_SOURCE=3` outside Debug, `-fcf-protection=full` on x86-64,
position-independent code and `-Wl,-z,relro,-z,now` (not on Apple platforms); with MSVC it is `/GS /guard:cf /sdl`
and `/GUARD:CF /DYNAMICBASE /NXCOMPAT /CETCOMPAT`. The test executables do not carry it. On Linux the test
`hardening.binary` reads the built command line back with `readelf` (PIE, `GNU_RELRO`, `BIND_NOW`, a stack
protector in the code) and the game library for the control-flow note, so a flag that a compiler ignored fails
a build and not a review. The Windows and Apple flags have no such check yet.

## Supply chain

- Every workflow action is pinned to a full commit SHA, with the shared checks and `zizmor` run on
  `.github/` changes and nightly.
- Dependencies come from vcpkg at a pinned baseline in the organisation's fork; Dependabot proposes
  updates after a seven-day delay and `Scan dependency diff` blocks a vulnerable one.
- Release files carry a SHA-512 list, a detached GPG signature checked against the committed public key,
  GitHub build-provenance attestations and a CycloneDX SBOM ([Releasing](releasing.md)).
- Commits are signed off (DCO) and every change goes through a pull request and the merge queue.

## Not covered

A device that is already compromised, a person with physical access to an unlocked device, and the
operating system's own backups of the application's data (an open question for iOS).
