# Release signing key (public half)

This directory holds the **public** half of the key that signs releases, once it
exists:

| File | What it is |
| --- | --- |
| `KEYS` | the ASCII-armoured public key |
| `FINGERPRINT` | its 40-hex-digit fingerprint, one line |

**Neither file is here yet.** They are written by
`tools/release/new_signing_key.ps1 -Install` (or `new_signing_key.sh --install`),
which the author runs once on their own machine; the script also writes, beside
the key it makes, a README.md with the `gh secret set` commands that upload the matching private key as the repository
secret `GPG_PRIVATE_KEY` (and `GPG_PASSPHRASE`). `release.yml` stops with those
steps while either file or either secret is missing, and refuses to sign with a
key whose fingerprint is not the one in `FINGERPRINT`.

The private key is never in the repository. The fingerprint is also meant to be
published somewhere a reader trusts independently of the download (`SECURITY.md`,
the README), because a signature checked against a key from the same place as the
file proves only that the two agree. How a user checks a download:
[docs/ci.md](../../docs/ci.md#checking-a-download).
