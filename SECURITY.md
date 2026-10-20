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

Today the programs read the files and arguments you give them and write to the
console or a window; the project's own code opens no network connections. So
what counts, at present, is a crash, hang or memory-safety fault that a crafted
board, word list or command line can cause, and a flaw in the build and
workflow files in this repository.

Camera input and reading letters from a picture are not built yet. When they
are, a picture will be untrusted input too, and this section will say so.

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

There has been no release yet. When there is one, this section will say which
versions receive fixes. The first release will be signed as above.
