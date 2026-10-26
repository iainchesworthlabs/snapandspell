# Releasing

A `v*` tag on `develop` becomes a GitHub Release. `release.yml` builds, tests and packages the
files; the shared release workflow of `icl-common-infra` checks the tag, signs, attests, publishes
and moves `main`. Nothing here runs on a pull request or in the merge queue, and none of it is a
required check. The iOS and Android builds are described in [CI](ci.md#ios).

## Cutting a release

1. Merge pull requests into `develop`. Each user-visible change brings a changelog fragment.
2. In one pull request set `project(... VERSION x.y.z)` in `CMakeLists.txt` and `"version"` in
   `vcpkg.json`, and assemble the changelog:
   `python3 <icl-common-infra>/scripts/changelog_tool.py assemble --version x.y.z --date YYYY-MM-DD`.
   A final release needs the `## [x.y.z]` section this creates. A pre-release (`v0.1.0-rc.1`) can
   skip the assembly: its notes are the fragments still in `changelog.d/`, which stay for the final
   release. `changelog_tool.py notes --version x.y.z --allow-pending` prints what a release would say.
3. Optional, and worth doing for the first release: tag `v0.1.0-rc.1` on `develop`. It takes the real
   path (the real key, attestations, a published pre-release) without making a final release.
4. Tag the merge commit and push the tag:

   ```sh
   git tag -s v0.1.0 -m "SnapAndSpell 0.1.0"
   git push origin v0.1.0
   ```

5. The `Publish` job waits for your approval in the `release` environment. When it is green the release
   is public and, for a final release, `main` has been fast-forwarded to the tag. The push to `main`
   publishes the documentation site.

A packaging fix after a tag is a new tag (`v0.1.1`). The `tags` ruleset refuses moving or deleting a
`v*` tag, and a moved tag would make the signatures and attestations of the first describe something
that is no longer there.

## What a release holds

| Platform | Built on | Packages |
| --- | --- | --- |
| Windows x64 | `windows-2025-vs2026`, MSVC, Qt 6.12.0 | `SnapAndSpell-<v>-win64<s>.zip`, `...-win64<s>.exe` (NSIS installer): the application and `sns-cli`. `<s>` is `-unsigned`, `-testsigned` or nothing ([Windows signing](#windows-signing)) |
| Linux x64, the application | `ubuntu-26.04`, GCC, Qt 6.12.0 | `SnapAndSpell-<v>-x86_64.AppImage`, with Qt inside |
| Linux x64, the solver | `ubuntu-26.04`, GCC | `.zip`, `.tar.gz`, `.deb`, `.rpm` of `sns-cli` only |
| macOS arm64 | `macos-26`, AppleClang, Qt 6.12.0 | `SnapAndSpell-<v>-Darwin.dmg`: the application bundle and `sns-cli`; signed and notarised when the Apple secrets are set |
| iPhone / iPad | `macos-26`, Xcode, Qt's iOS kit | no file: the build goes to TestFlight, when `SIGN_IOS_UPLOAD` is true ([iOS](ci.md#ios)) |
| Android arm64 | `ubuntu-26.04`, Qt's Android kit | `...-android-arm64.apk` and `.aab`, when the four keystore secrets are set ([Android](ci.md#android)) |
| Any | the tagged commit | `SnapAndSpell-<v>-source.tar.gz` (`git archive`, so `.gitattributes` decides what stays out) |

Beside them: `SnapAndSpell-<v>.cdx.json` (the CycloneDX 1.6 SBOM, [below](#the-sbom)),
`SHA512SUMS`, and a detached `.asc` signature for every file. A pre-release's files carry its version
(`...-0.1.0-rc.1-...`, a deb `0.1.0~rc.1`, an rpm release `0.rc.1`).

Release builds have Tracy off: the Release presets turn it off (`tests/profiling/check_presets.cmake`
holds that), and every build job fails if `CMakeCache.txt` has it on or a package it can list holds a
file with Tracy or a sanitizer runtime in its name.

Not built: a deb or rpm of the application (it needs a Qt 6.12 that no distribution packages; the
AppImage is the application), a production Authenticode signature on the Windows files (SmartScreen
warns on first run; the GPG signature and the attestation prove who built a file; see
[Windows signing](#windows-signing)), Intel macOS and Linux arm64 (no hosted runner in the matrix). Releasing to a public store is a separate licensing decision (D2).

## The jobs

| Job | What it does |
| --- | --- |
| `Prepare the release` | Decides the mode. For a tag, `tools/release/check_release_ref.sh` requires `vMAJOR.MINOR.PATCH[-prerelease]` and the same version in `CMakeLists.txt` and `vcpkg.json`. |
| `Build ...` (Windows, Linux solver, Linux application, macOS) | Configures the Release preset, builds, runs all of ctest, packages, smoke-tests what it packaged and checks what is in it (`check_release_build.ps1`: Tracy and sanitizers off, the expected packages present). |
| `Build the iOS App Store IPA for TestFlight`, `Build the signed Android packages` | A tag, or a manual run unless its input is unticked (the iOS one only when `SIGN_IOS_UPLOAD` is true; a manual run uploads to TestFlight only with `upload_testflight`). |
| `Source archive and SBOM` | Downloads the packages, archives the source, writes the SBOM. A rehearsal ends here. |
| `Publish` | The shared workflow. `Verify tag`: semver, on `develop`, the release has notes. `Sign and publish` (the `release` environment): `SHA512SUMS`, signatures checked against `docs/signing-key.asc`, attestations, the GitHub Release. `Fast-forward release branch`: for a final release, the release bot moves `main`. |
| `Add the install notes` | Appends the iPhone and Android install sections to the notes, when those files exist. |

A tag-triggered run uses the workflow file at the tagged commit, so a release is built by the
pipeline that was reviewed with it.

## Rehearsal

```sh
gh workflow run release.yml --ref develop
```

The same workflow, started by hand or weekly (Sunday 03:23 UTC), builds, tests, packages and writes the
SBOM, and signs and publishes nothing. The files are the run's `package-*` artifacts for 14 days. The
`ios` and `android` inputs (default true, by hand only) also build those packages with the real
secrets; untick them to rehearse the desktop packages alone. Unpack each package on its platform and
run it: `sns-cli`, and `SnapAndSpell --smoke 10` for the application. The weekly run exists so
that a Release build that has quietly stopped working is found on a Sunday, not on the day of a release.

## What the author sets up once

The organisation holds the release secrets (`REPO_GPG_PRIVATE_KEY`, `REPO_GPG_PASSPHRASE`,
`RELEASE_BOT_APP_ID`, `RELEASE_BOT_PRIVATE_KEY`), the `release` environment and the `tags` and
`main-release-only` rulesets. `admin/apply.sh` in `icl-common-infra` applies the environment and the
rulesets; the secrets are in its `docs/secrets.md`. The public key is `docs/signing-key.asc` and its
fingerprint is in [SECURITY.md](https://github.com/iainchesworthlabs/wordjewelssolver/blob/develop/SECURITY.md#verifying-a-release).
Replacing the key means a new `docs/signing-key.asc` and a new fingerprint in `SECURITY.md` and the
README in the same commit; old releases keep verifying against the old key, which stays in the history.

This repository adds:

- **iOS** (TestFlight): the organisation's `IOS_DIST_CERT_P12_BASE64`, `IOS_DIST_CERT_PASSWORD`,
  `APPSTORE_API_KEY_ID`, `APPSTORE_API_ISSUER_ID` and `APPSTORE_API_PRIVATE_KEY` (they must reach this
  repository), one repository secret, `IOS_PROVISIONING_PROFILE_BASE64` (an App Store profile, no devices; it
  lasts until the certificate expires, 2027-07-16), and the repository variable `SIGN_IOS_UPLOAD=true`
  ([Installing on an iPhone or iPad](ios-install.md#for-the-owner)). There is no iOS file on a release: a
  build that installs from a file would list its devices' identifiers, and the repository is public.
- **Android**: four repository secrets for the upload keystore, made with
  `tools/release/new_android_keystore.sh` ([Android](ci.md#android)). With none set the release has no
  Android file; with some set and not all, the job fails.
- **macOS signing** (optional): `MACOS_CERTIFICATE`, `MACOS_CERTIFICATE_PASSWORD`, `MACOS_SIGN_IDENTITY`,
  `NOTARY_APPLE_ID`, `NOTARY_TEAM_ID`, `NOTARY_PASSWORD`. Without them the disk image is unsigned and
  Gatekeeper warns.
- **The AppImage tools** are pinned by SHA-256 in `linuxdeploy.pins`. To bump one, change the tag in its
  URL, set its hash to `PENDING` and run `libs/platforms/linux/packaging/pin_linuxdeploy.sh` on a machine
  you trust.

## Windows signing

The Windows leg signs behind the repository variable `SIGN_WINDOWS` (standard section 13, D17), with the
shared `sign-windows` action of icl-common-infra:

| `SIGN_WINDOWS` | What the leg does | Package names |
| --- | --- | --- |
| `false` (the default; unset counts as `false`) | nothing is signed | `...-win64-unsigned.zip`, `...-win64-unsigned.exe` |
| `test` | `SnapAndSpell.exe` and `sns-cli.exe` are signed where they are built, so the ZIP and the installer carry the signed files; the installer is signed too; every signature has an RFC 3161 timestamp | `...-win64-testsigned.zip`, `...-win64-testsigned.exe` |
| `production` | the action fails until a signing service is chosen and wired (D17, open) | none |

`test` needs the organisation secrets `WIN_TESTSIGN_PFX_BASE64` and `WIN_TESTSIGN_PFX_PASSWORD`: the
leaf certificate of the owner's personal CA, never the CA's key. It is trusted only on a machine that has
the CA's root installed, so the release notes must say that a `-testsigned` file is a test signature
([test signing](https://github.com/iainchesworthlabs/icl-common-infra/blob/develop/docs/test-signing.md)).
Qt's own DLLs in the ZIP and the installer are not re-signed. A rehearsal signs and names the files the
same way, so the mode can be tried with a dispatch before a tag. Changing the variable is the owner's
(Settings, Secrets and variables, Actions, Variables).

## Checking a download

With the files in one folder:

```sh
gpg --import docs/signing-key.asc
gpg --fingerprint FD13CB8CAC8316648CD974D08149254C41894DBE   # compare with SECURITY.md
gpg --verify SHA512SUMS.asc SHA512SUMS
sha512sum -c SHA512SUMS
gh attestation verify SnapAndSpell-0.1.0-win64-unsigned.zip --repo iainchesworthlabs/wordjewelssolver
```

## The SBOM

`tools/release/generate_sbom.ps1` writes a CycloneDX 1.6 document from what the repository already
records, not from a scan of the binaries:

- the application and `sns-cli` (both GPL-3.0-or-later), with the
  commit and the repository;
- Qt by module at the version in `.github/toolchain-versions.json`, under the licence it is used
  under, only if a ZIP in the release carries `Qt6Core.dll` or a package is an AppImage, a disk image
  or an iOS IPA (which carry Qt by construction); likewise the Microsoft Visual C++ runtime. The
  command-line packages claim no Qt;
- the data that ships: the ENABLE word list and each bundled font file, with SHA-256 hashes computed
  from the files in the tree;
- what only built or tested it, with `scope: excluded`: vcpkg, CMake, Ninja and every port vcpkg
  installed, read from the `vcpkg.spdx.json` files vcpkg writes;
- each release file as a `distribution` reference with its SHA-256.

It states that it is incomplete for third parties: Qt contains third-party code that the SPDX
documents of the Qt kit describe. The output is deterministic for a given tag and commit.
`release-tooling.yml` validates a generated SBOM against the CycloneDX 1.6 JSON Schema.

## When a release run goes red

| Message | Cause |
| --- | --- |
| `the tag ... is version X but CMakeLists.txt declares Y` | The release pull request did not change `project(VERSION)` or `vcpkg.json`, or the tag is on the wrong commit. Delete the tag (admin), fix, tag again. |
| `... has no release notes: CHANGELOG.md has no '## [x.y.z]' section` | Assemble the changelog in a pull request (step 2). |
| `... does not point at a commit on develop` | A tag is made on a commit that `develop` contains. For a try-out, use a `-rc` tag on `develop`. |
| `missing organisation secrets` | The organisation secrets are not available to this repository (`docs/secrets.md` in `icl-common-infra`). |
| `docs/signing-key.asc is missing`, or a signature does not verify against it | The committed key is not the key that signed. |
| `SNS_ENABLE_TRACY is 'ON'` / `contains the profiler` | A Release configuration or package has Tracy in it. The presets decide; `tests/profiling/check_presets.cmake` should have failed first. |
| `no package ending .rpm` (and similar) | CPack dropped a generator because its tool was missing at configure time. Read the configure log's `Packaging:` line. |
| `makensis.exe is not in ...` | The NSIS install failed. NSIS is pinned to 3.13.0 from Chocolatey. |

## Not verified until it runs on GitHub

`release.yml` has never run on GitHub. The Release build, the whole ctest suite, the ZIP package check
and the SBOM against its schema have run (locally and in `Package (Windows, Release)` on every push).
Written, linted and reasoned, and not yet run: the Linux and macOS desktop Qt builds, the AppImage,
the macOS bundle checks and signing, the NSIS installer's branding and silent install, the iOS signing,
archive and export (only the unsigned compile has run), the Android signing, and the shared `Publish`
job in this repository. The first rehearsal is expected to need a fix or two; that is what it is for.
