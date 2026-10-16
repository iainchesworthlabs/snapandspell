#!/usr/bin/env pwsh
#requires -Version 7.0
#
# Makes the project's own dice for the standard board, from the letter counts of
# the dictionary, and writes them to data/dice/<n>x<n>.txt.
#
# Why: the faces of a printed set may belong to someone, and a list found online
# has no origin (docs/design/board.md, "Dice"). These are the project's own work
# from the list the repository already records as public domain. Nothing here
# is any product's distribution of letters; the counts are the dictionary's.
#
# Method, which is also written into the header of each file:
#
#   - The letter counts are those of data/dictionaries/en.txt: how many times
#     each of A to Z occurs in its words.
#   - A die has six different letters, dealt in proportion to those counts, a
#     letter already on the die taken out of the draw.
#   - A die that has no vowel (A E I O U) is dealt again.
#   - A Q is the two-letter tile QU, as on a Boggle die.
#   - A size n by n has n * n dice. Each size has its own seed and its own dice:
#     the 3x3 is not a window on the 4x4.
#   - The draw is a 32-bit linear congruential generator, so the files can be
#     made again on any machine: state = state * 1664525 + 1013904223 mod 2^32,
#     and a draw below n is (state >> 8) mod n.
#
# The header records the word list's SHA-256, so a dictionary that changes makes
# the dice stale and -Check says so.
#
# Usage:  ./tools/data/make_dice.ps1 [-Root <repo>] [-Size 3,4,5,6]   write
#         ./tools/data/make_dice.ps1 -Check                           compare, write nothing
# Exit:   0 = written or matching, 1 = a file differs or is missing, 2 = bad invocation.

param(
    [string]$Root = (Join-Path $PSScriptRoot '../..'),
    [int[]]$Size = @(3, 4, 5, 6),
    [switch]$Check
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path -LiteralPath $Root).Path
$source = Join-Path $Root 'data/dictionaries/en.txt'
$outputDirectory = Join-Path $Root 'data/dice'

if (-not (Test-Path -LiteralPath $source)) {
    Write-Error "No data/dictionaries/en.txt under '$Root'."
    exit 2
}

$facesPerDie = 6
$vowels = 'A', 'E', 'I', 'O', 'U'

# --- the counts -------------------------------------------------------------

# The hash is of the file with its carriage returns taken out, so that a checkout
# that turned line feeds into CRLF (a Windows one) has the hash data/README.md
# records, which is the file as fetched.
$utf8 = [System.Text.UTF8Encoding]::new($false)
$plainText = $utf8.GetString([System.IO.File]::ReadAllBytes($source)).Replace("`r", '')
$sourceHash = [System.BitConverter]::ToString(
    [System.Security.Cryptography.SHA256]::HashData($utf8.GetBytes($plainText))).Replace('-', '')
$text = $plainText.ToUpperInvariant()

$letters = [char[]](65..90)
$counts = [int[]]::new(26)
for ($i = 0; $i -lt 26; $i++) {
    $counts[$i] = $text.Length - $text.Replace([string]$letters[$i], '').Length
}

# --- the generator ------------------------------------------------------------

$script:state = [uint64]0

function Set-Seed([int]$seed) { $script:state = [uint64]$seed -band 0xFFFFFFFF }

# A draw below $n: step the generator, take its high bits.
function Get-Draw([int]$n) {
    $script:state = ($script:state * 1664525 + 1013904223) % 4294967296
    return [int](($script:state -shr 8) % $n)
}

# One die: six different letters dealt in proportion to the counts, with a vowel.
function New-Die {
    while ($true) {
        $weights = [int[]]$counts.Clone()
        $faces = [System.Collections.Generic.List[string]]::new()
        for ($face = 0; $face -lt $facesPerDie; $face++) {
            $total = 0
            foreach ($w in $weights) { $total += $w }
            $pick = Get-Draw $total
            $running = 0
            for ($i = 0; $i -lt 26; $i++) {
                $running += $weights[$i]
                if ($running -gt $pick) {
                    $weights[$i] = 0
                    $faces.Add([string]$letters[$i])
                    break
                }
            }
        }
        if (@($faces | Where-Object { $vowels -contains $_ }).Count -gt 0) {
            $tiles = $faces | ForEach-Object { if ($_ -eq 'Q') { 'QU' } else { $_ } } | Sort-Object
            return ($tiles -join ' ')
        }
    }
}

function New-DiceFile([int]$side) {
    $seed = 7919 * $side
    Set-Seed $seed
    $dice = 1..($side * $side) | ForEach-Object { New-Die }
    $header = @(
        "# dice for a ${side}x${side} board: $($side * $side) dice of $facesPerDie faces, one a line"
        '# made-by: tools/data/make_dice.ps1'
        '# source: data/dictionaries/en.txt'
        "# source-sha256: $sourceHash"
        '# method: six different letters to a die, dealt in proportion to the source''s letter counts,'
        '#   at least one of A E I O U on each, Q as the tile QU, faces of a die in letter order'
        '# prng: 32-bit LCG, state = state * 1664525 + 1013904223 mod 2^32; a draw below n is (state >> 8) mod n'
        "# seed: $seed"
    )
    return (($header + $dice) -join "`n") + "`n"
}

# --- write, or compare --------------------------------------------------------

$problems = [System.Collections.Generic.List[string]]::new()
foreach ($side in $Size) {
    if ($side -lt 1 -or $side -gt 16) {
        Write-Error "A size is 1 to 16, not $side."
        exit 2
    }
    $name = "${side}x${side}.txt"
    $path = Join-Path $outputDirectory $name
    $wanted = New-DiceFile $side

    if ($Check) {
        if (-not (Test-Path -LiteralPath $path)) {
            $problems.Add("data/dice/${name}: missing; run tools/data/make_dice.ps1")
            continue
        }
        $actual = [System.IO.File]::ReadAllText($path).Replace("`r`n", "`n")
        if ($actual -cne $wanted) {
            $problems.Add("data/dice/${name}: is not what tools/data/make_dice.ps1 makes from data/dictionaries/en.txt (its hash or its dice differ); run the tool again")
        }
    } else {
        New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
        [System.IO.File]::WriteAllText($path, $wanted, [System.Text.UTF8Encoding]::new($false))
        Write-Host "wrote data/dice/$name ($($side * $side) dice)"
    }
}

if ($problems.Count -gt 0) {
    Write-Host ''
    Write-Host 'Dice problem(s):' -ForegroundColor Red
    foreach ($problem in $problems) {
        Write-Host "  $problem"
        $file = $problem.Substring(0, $problem.IndexOf(':'))
        Write-Host "::error file=${file}::$($problem.Substring($problem.IndexOf(':') + 1).Trim())"
    }
    exit 1
}

if ($Check) {
    Write-Host "OK: $($Size.Count) dice file(s) are what tools/data/make_dice.ps1 makes from the dictionary (source SHA-256 $($sourceHash.Substring(0, 12))...)." -ForegroundColor Green
}
exit 0
