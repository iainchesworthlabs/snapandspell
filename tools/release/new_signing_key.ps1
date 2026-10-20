#!/usr/bin/env pwsh
#requires -Version 7.0
<#
.SYNOPSIS
    Create the release signing key pair, ON YOUR OWN MACHINE, and print the
    exact steps that put it to work. A PowerShell front for new_signing_key.sh.

.DESCRIPTION
    The repository's workflow never makes a key: it only reads one from two
    repository secrets (docs/ci.md, "Releases"). The work is done by
    new_signing_key.sh, which reads: what it does, what it writes and what it
    prints. This script finds a bash (Git for Windows ships one, and a gpg
    that goes with it) and runs that script with the same options, because
    GnuPG's agent does not start reliably when a Windows-native PowerShell
    drives Git for Windows' gpg, and a key script that sometimes does not
    start is the wrong thing to be clever about.

    Nothing is uploaded and nothing is committed. The keyring used is a
    temporary one, removed on exit.

.PARAMETER Out
    Output directory; must not exist or must be empty. Default ./release-key-out.

.PARAMETER Uid
    The key's name. Default "WordJewelsSolver release signing key".

.PARAMETER Email
    Appended to the name as <address>. Optional.

.PARAMETER Expire
    gpg expiry: 2y (default), 18m, 0 for none.

.PARAMETER PassphraseFile
    Use this passphrase instead of a random one.

.PARAMETER Repo
    owner/name the printed gh commands name.

.PARAMETER Install
    Copy the public key and fingerprint to .github/release-signing/.

.EXAMPLE
    ./tools/release/new_signing_key.ps1 -Email you@example.org -Install
#>
[CmdletBinding()]
param(
    [string]$Out = 'release-key-out',
    [string]$Uid = '',
    [string]$Email = '',
    [string]$Expire = '',
    [string]$PassphraseFile = '',
    [string]$Repo = '',
    [switch]$Install
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Find-Bash {
    $candidates = @(
        (Join-Path $env:ProgramFiles 'Git\bin\bash.exe'),
        (Join-Path $env:ProgramFiles 'Git\usr\bin\bash.exe')
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }
    $command = Get-Command bash -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    return $null
}

$bash = Find-Bash
if (-not $bash) {
    throw 'No bash found. Install Git for Windows (it brings bash and gpg) and run again, or run tools/release/new_signing_key.sh from any POSIX shell that has gpg.'
}

$script = (Resolve-Path (Join-Path $PSScriptRoot 'new_signing_key.sh')).Path
$arguments = @('--out', $Out)
if ($Uid) { $arguments += @('--uid', $Uid) }
if ($Email) { $arguments += @('--email', $Email) }
if ($Expire) { $arguments += @('--expire', $Expire) }
if ($PassphraseFile) { $arguments += @('--passphrase-file', $PassphraseFile) }
if ($Repo) { $arguments += @('--repo', $Repo) }
if ($Install) { $arguments += '--install' }

# bash gets the script as a path it understands; the other paths pass through
# unchanged, which Git's bash accepts in Windows form.
$scriptForBash = $script -replace '\\', '/'
& $bash $scriptForBash @arguments
exit $LASTEXITCODE
