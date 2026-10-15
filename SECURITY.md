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

## Supported versions

There has been no release yet. When there is one, this section will say which
versions receive fixes.
