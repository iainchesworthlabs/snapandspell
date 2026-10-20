# Release signing key (public half)

This directory holds the **public** half of the key that signs releases:

| File | What it is |
| --- | --- |
| `KEYS` | the ASCII-armoured public key |
| `FINGERPRINT` | its 40-hex-digit fingerprint, one line |

Both files are here: the key is `9A0D96E8944899F94B1B37B51F1A8DA4C79B19D2`
(`WordJewelsSolver Release <iain.chesworth@gmail.com>`, Ed25519, created
2026-10-20, expires 2028-10-19). They were written by
`tools/release/new_signing_key.ps1` (or `new_signing_key.sh`), which the author
ran once on their own machine; it also writes, beside the key it made, a
README.md with the `gh secret set` commands that upload the matching private key
as the repository secret `GPG_PRIVATE_KEY` (and `GPG_PASSPHRASE`). `release.yml`
stops with those steps if either secret is missing, and refuses to sign with a
key whose fingerprint is not the one in `FINGERPRINT`.

The private key is never in the repository. The fingerprint is also meant to be
published somewhere a reader trusts independently of the download (`SECURITY.md`,
the README), because a signature checked against a key from the same place as the
file proves only that the two agree. How a user checks a download:
[docs/ci.md](../../docs/ci.md#checking-a-download).
