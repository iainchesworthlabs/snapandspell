# Security policy

## Reporting a vulnerability

Please report security vulnerabilities **privately**, not as a public issue or
pull request, through GitHub's private vulnerability reporting: open the
repository's **Security** tab and choose **Report a vulnerability**. It opens a
private advisory that only you and the maintainer can see.

What helps most:

- what is affected: the version (`wjs-cli --version` prints it), the platform,
  and whether it is `wjs-cli`, the Qt Quick window or the library;
- how to reproduce it: the board, the word list (or a small one that shows it)
  and the command line;
- what someone could do with it.

Please do not attach a picture of a person or a room.

If you cannot use the form (you have no GitHub account, say), open an issue that
says only that you have a security matter, with no details, and the maintainer
will reply there to arrange a private channel.

### What counts

The programs read the files and arguments you give them, a camera picture and
the application's own store file, and write to the console, a window and that
store; the project's own code opens no network connections. So what counts is:

- a crash, hang or memory-safety fault that a crafted board, word list, command
  line or **store file** can cause;
- the same from a **camera frame**. A frame is untrusted input: Mode A hands the
  camera's picture to the board reader (`libs/scan`), which validates its size and
  format before it reads a pixel (a hostile frame size cannot wrap the checking
  arithmetic, and an invalid frame reads as an empty picture). The reader has been
  tested on drawn frames and on malformed ones, and has never been fuzzed or seen
  a hostile real frame; a frame is processed in memory and never written to disk
  or sent anywhere, and a report that it is would count too;
- a flaw in the build and workflow files in this repository.

**What does not count, and is not a fault.** A *profiling* build is not a
release. The Debug and profiling presets build the Tracy profiler into the
programs, and a Tracy client **opens a TCP listener on port 8086 on every
interface** and answers discovery broadcasts for as long as the program runs
([docs/profiling.md](docs/profiling.md#the-profiler-listens-on-the-network)).
The release presets build without it, `cpack` refuses to package a Tracy build,
and `tools/package/check_package.ps1` rejects a package that carries a Tracy
client; but the CI artefact of a Debug build, and your own Debug build, have
the port. Do not run one on an untrusted network.

### What to expect

WordJewelsSolver is maintained by one person, so there is no guaranteed response
time and no promised schedule for a fix. Please allow time before you publish
details: the advisory is where that is agreed.

## Verifying a release

Releases are signed. Every file attached to a release has a detached GPG
signature (`.asc`), and `SHA256SUMS` lists them all. The signing key is:

```text
WordJewelsSolver Release <iain.chesworth@gmail.com>
Ed25519, created 2026-10-20, expires 2028-10-19
Fingerprint: 9A0D 96E8 9448 99F9 4B1B  37B5 1F1A 8DA4 C79B 19D2
             (9A0D96E8944899F94B1B37B51F1A8DA4C79B19D2)
```

Trust the fingerprint above, which you are reading in the repository, and not a
key file that arrives with a download: a signature checked against a key from
the same place as the file proves only that the two agree. The public key is
`.github/release-signing/KEYS` in this repository. To check a download, follow
[docs/ci.md, "Checking a download"](docs/ci.md#checking-a-download); in short:

```sh
tools/release/verify_release.sh --dir <folder with the files> --fingerprint 9A0D96E8944899F94B1B37B51F1A8DA4C79B19D2
```

Builds also carry GitHub build-provenance attestations
(`gh attestation verify <file> --repo iainchesworthlabs/wordjewelssolver`) and a
CycloneDX software bill of materials. If this key is ever replaced or revoked,
this section changes in the same commit that changes `KEYS`.

## Supported versions

There has been no release yet (0.1.0 is being prepared). When there is one, this
section will say which versions receive fixes. The first release will be signed
as above, and until a release exists, `develop` is the only supported line.
