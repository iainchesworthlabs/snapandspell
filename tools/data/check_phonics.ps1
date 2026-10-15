#!/usr/bin/env pwsh
#requires -Version 7.0
#
# Holds the phonics phase files under data/phonics to the rules the
# application relies on (docs/testing.md, "Phonics data"). It reads the same
# format as src/lib/src/phase_data.cpp and applies the same checks, so a file
# that passes here loads there.
#
#   data/phonics/phase-<x>.graphemes.txt   one grapheme a line:  ai  /sound/  clip:ai
#   data/phonics/phase-<x>.words.txt       one word a line:      pain  p ai n
#                                                                 <word>  tricky
#
# Each file starts with `# phase: <x>` and may carry
# `# reviewed-by: <name>   reviewed-on: <date>   source: <where it came from>`.
# A file without both reviewed-by and reviewed-on (and reviewed-on as
# YYYY-MM-DD) is test data. -RequireReviewed makes that a failure, which is the
# release build's gate (WJS_REQUIRE_REVIEWED_PHONICS).
#
# Checks, each reported with file and line:
#   - the file name is phase-<x>.graphemes.txt or phase-<x>.words.txt and the
#     header names the same phase;
#   - the line grammar; lowercase a-z words of three or more letters, graphemes
#     of lowercase letters, sounds between slashes, clip ids;
#   - no word or grapheme twice, in a file or across phases; sorted (byte order);
#   - every word is in data/dictionaries/en.txt;
#   - the graphemes of a word, joined, spell the word (not for `tricky`);
#   - every grapheme of a word is in the word's phase or an earlier one
#     (phases are cumulative, ordered by name).
#
# No data/phonics directory, or none with a .txt file in it, is not a problem:
# there is nothing to check.
#
# Exit: 0 = clean, 1 = violation(s), 2 = bad invocation.

param(
    [string]$Root = (Join-Path $PSScriptRoot '../..'),
    [switch]$RequireReviewed
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path -LiteralPath $Root).Path
$phonicsDir = Join-Path $Root 'data/phonics'
$english = Join-Path $Root 'data/dictionaries/en.txt'

$minWordLetters = 3

# --- what there is -----------------------------------------------------------

$files = @()
if (Test-Path -LiteralPath $phonicsDir) {
    $files = @(Get-ChildItem -LiteralPath $phonicsDir -Filter *.txt -File | Sort-Object Name)
}
if ($files.Count -eq 0) {
    Write-Host 'OK: no phonics data under data/phonics, so nothing to check.' -ForegroundColor Green
    exit 0
}
if (-not (Test-Path -LiteralPath $english)) {
    Write-Error "No data/dictionaries/en.txt under '$Root'."
    exit 2
}

$problems = [System.Collections.Generic.List[object]]::new()
function Add-Problem([string]$file, [int]$line, [string]$message) {
    $script:problems.Add([pscustomobject]@{ File = $file; Line = $line; Message = $message })
}

function Test-IsoDate([string]$text) {
    $parsed = [datetime]::MinValue
    return $text -cmatch '^[0-9]{4}-[0-9]{2}-[0-9]{2}$' -and [datetime]::TryParseExact(
        $text, 'yyyy-MM-dd', [System.Globalization.CultureInfo]::InvariantCulture,
        [System.Globalization.DateTimeStyles]::None, [ref]$parsed)
}

# The lines of a file, without the byte-order mark and the carriage returns of
# a Windows editor.
function Read-DataLines([string]$path) {
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $text = [System.Text.UTF8Encoding]::new($false).GetString($bytes)
    if ($text.Length -gt 0 -and $text[0] -eq [char]0xFEFF) { $text = $text.Substring(1) }
    return @($text -split "`n" | ForEach-Object { $_.TrimEnd("`r") })
}

# A file read: its name, phase, kind, header and entries. Entries are objects
# with Line and, for words, Word, Graphemes and Tricky; for graphemes,
# Letters, Sound and Clip.
function Read-PhaseFile([System.IO.FileInfo]$file) {
    $name = $file.Name
    $result = [pscustomobject]@{
        Name = $name; Phase = ''; Kind = ''; Reviewed = $false; Entries = @(); Ok = $true
    }

    $match = [regex]::Match($name, '^phase-([A-Za-z0-9]+)\.(graphemes|words)\.txt$')
    if (-not $match.Success) {
        Add-Problem $name 0 'not named phase-<x>.graphemes.txt or phase-<x>.words.txt'
        $result.Ok = $false
        return $result
    }
    $nameCase = $match.Groups[1].Value.ToUpperInvariant()
    $result.Kind = $match.Groups[2].Value

    $header = @{ phase = ''; 'reviewed-by' = ''; 'reviewed-on' = ''; source = '' }
    $seen = @{}
    $entries = [System.Collections.Generic.List[object]]::new()
    $firstLine = @{}
    $previous = ''
    $lineNumber = 0

    foreach ($raw in (Read-DataLines $file.FullName)) {
        $lineNumber++
        $text = $raw.Trim(" `t")
        if ($text.Length -eq 0) { continue }

        if ($text.StartsWith('#')) {
            $comment = $text.Substring(1)
            $hits = [regex]::Matches($comment, '(?<=^|[ \t])(phase|reviewed-by|reviewed-on|source):')
            for ($i = 0; $i -lt $hits.Count; $i++) {
                $key = $hits[$i].Groups[1].Value
                $start = $hits[$i].Index + $hits[$i].Length
                $end = if ($i + 1 -lt $hits.Count) { $hits[$i + 1].Index } else { $comment.Length }
                $value = $comment.Substring($start, $end - $start).Trim(" `t")
                if ($seen.ContainsKey($key)) {
                    Add-Problem $name $lineNumber "'${key}:' is given twice"
                    continue
                }
                $seen[$key] = $true
                if ($value -cmatch '^<.*>$') { $value = '' }
                if ($key -eq 'phase') {
                    if ($value -cmatch '^[A-Za-z0-9]+$') {
                        $header['phase'] = $value.ToUpperInvariant()
                    } else {
                        Add-Problem $name $lineNumber "the phase must be letters and digits, as in '# phase: A'"
                        $result.Ok = $false
                    }
                } else {
                    $header[$key] = $value
                }
            }
            continue
        }

        $fields = @($text -split '[ \t]+' | Where-Object { $_ })

        if ($result.Kind -eq 'graphemes') {
            if ($fields.Count -ne 3) {
                Add-Problem $name $lineNumber "a grapheme line is the letters, the sound and the clip: 'ai  /sound/  clip:ai'"
                $result.Ok = $false
                continue
            }
            $letters, $sound, $clip = $fields
            $good = $true
            if ($letters -cnotmatch '^[a-z]+$') {
                Add-Problem $name $lineNumber "'$letters' is not lowercase a-z letters (a split digraph cannot be written yet)"
                $good = $false
            }
            if ($sound -cnotmatch '^/[^/]+/$') {
                Add-Problem $name $lineNumber "'$sound' is not a sound written between slashes"
                $good = $false
            }
            if ($clip -cnotmatch '^clip:[a-z0-9._-]+$') {
                Add-Problem $name $lineNumber "'$clip' is not a clip id written 'clip:<id>'"
                $good = $false
            }
            if (-not $good) { $result.Ok = $false; continue }
            if ($firstLine.ContainsKey($letters)) {
                Add-Problem $name $lineNumber "'$letters' is listed twice (first on line $($firstLine[$letters]))"
                $result.Ok = $false
                continue
            }
            if ($previous -ne '' -and [string]::CompareOrdinal($previous, $letters) -gt 0) {
                Add-Problem $name $lineNumber "'$letters' is out of order (after '$previous')"
                $result.Ok = $false
            }
            $previous = $letters
            $firstLine[$letters] = $lineNumber
            $entries.Add([pscustomobject]@{ Line = $lineNumber; Letters = $letters; Sound = $sound; Clip = $clip })
            continue
        }

        # A words file.
        $word = $fields[0]
        if ($word -cnotmatch '^[a-z]+$') {
            Add-Problem $name $lineNumber "'$word' is not a lowercase a-z word"
            $result.Ok = $false
            continue
        }
        if ($word.Length -lt $minWordLetters) {
            Add-Problem $name $lineNumber "'$word' has fewer than $minWordLetters letters, so no board can count it"
            $result.Ok = $false
            continue
        }
        if ($fields.Count -eq 1) {
            Add-Problem $name $lineNumber "'$word' has no graphemes: write them after the word, or mark the word 'tricky'"
            $result.Ok = $false
            continue
        }
        $tricky = $false
        $graphemes = @()
        if ($fields.Count -eq 2 -and $fields[1] -ceq 'tricky') {
            $tricky = $true
        } else {
            $good = $true
            foreach ($grapheme in $fields[1..($fields.Count - 1)]) {
                if ($grapheme -ceq 'tricky') {
                    Add-Problem $name $lineNumber "'$word': 'tricky' stands alone, with no split"
                    $good = $false
                    break
                }
                if ($grapheme -cnotmatch '^[a-z]+$') {
                    Add-Problem $name $lineNumber "'$grapheme' in '$word' is not lowercase a-z letters (a split digraph cannot be written yet)"
                    $good = $false
                    break
                }
                $graphemes += $grapheme
            }
            if (-not $good) { $result.Ok = $false; continue }
        }
        if ($firstLine.ContainsKey($word)) {
            Add-Problem $name $lineNumber "'$word' is listed twice (first on line $($firstLine[$word]))"
            $result.Ok = $false
            continue
        }
        if ($previous -ne '' -and [string]::CompareOrdinal($previous, $word) -gt 0) {
            Add-Problem $name $lineNumber "'$word' is out of order (after '$previous')"
            $result.Ok = $false
        }
        $previous = $word
        $firstLine[$word] = $lineNumber
        $entries.Add([pscustomobject]@{ Line = $lineNumber; Word = $word; Graphemes = $graphemes; Tricky = $tricky })
    }

    # What the header as a whole says.
    if (-not $seen.ContainsKey('phase')) {
        Add-Problem $name 0 "no '# phase: <name>' header"
        $result.Ok = $false
    } elseif ($header['phase'] -ne '' -and $header['phase'] -cne $nameCase) {
        Add-Problem $name 0 "its header says phase $($header['phase']), its name says phase $nameCase"
        $result.Ok = $false
    }
    $by = $header['reviewed-by'] -ne ''
    $on = $header['reviewed-on'] -ne ''
    if ($by -ne $on) {
        Add-Problem $name 0 'give both reviewed-by and reviewed-on, or neither (neither is test data)'
        $result.Ok = $false
    }
    if ($on -and -not (Test-IsoDate $header['reviewed-on'])) {
        Add-Problem $name 0 'reviewed-on must be a date written YYYY-MM-DD'
        $result.Ok = $false
    }

    $result.Phase = $header['phase']
    $result.Reviewed = $by -and $on -and (Test-IsoDate $header['reviewed-on'])
    $result.Entries = $entries.ToArray()
    return $result
}

# --- read every file ---------------------------------------------------------

$parsed = @($files | ForEach-Object { Read-PhaseFile $_ })

# --- the cross-file checks ---------------------------------------------------

[string[]]$englishWords = [System.IO.File]::ReadAllLines($english)
$englishSet = [System.Collections.Generic.HashSet[string]]::new($englishWords, [System.StringComparer]::Ordinal)

$usable = @($parsed | Where-Object { $_.Ok -and $_.Phase -ne '' })

$names = [System.Collections.Generic.SortedSet[string]]::new([System.StringComparer]::Ordinal)
$graphemeFile = @{}
$wordFile = @{}
foreach ($file in $usable) {
    [void]$names.Add($file.Phase)
    $table = if ($file.Kind -eq 'graphemes') { $graphemeFile } else { $wordFile }
    if ($table.ContainsKey($file.Phase)) {
        Add-Problem $file.Name 0 "phase $($file.Phase) has a second $($file.Kind) file (the first is $($table[$file.Phase].Name))"
    } else {
        $table[$file.Phase] = $file
    }
}

$knownGraphemes = @{}
$knownWords = @{}
$graphemeCount = 0
$wordCount = 0
foreach ($phase in $names) {
    if ($graphemeFile.ContainsKey($phase)) {
        $file = $graphemeFile[$phase]
        foreach ($entry in $file.Entries) {
            if ($knownGraphemes.ContainsKey($entry.Letters)) {
                Add-Problem $file.Name $entry.Line "'$($entry.Letters)' is already defined ($($knownGraphemes[$entry.Letters]))"
            } else {
                $knownGraphemes[$entry.Letters] = "$($file.Name):$($entry.Line)"
                $graphemeCount++
            }
        }
    }
    if ($wordFile.ContainsKey($phase)) {
        $file = $wordFile[$phase]
        foreach ($entry in $file.Entries) {
            if (-not $englishSet.Contains($entry.Word)) {
                Add-Problem $file.Name $entry.Line "'$($entry.Word)' is not in the dictionary"
            }
            if (-not $entry.Tricky) {
                foreach ($grapheme in $entry.Graphemes) {
                    if (-not $knownGraphemes.ContainsKey($grapheme)) {
                        Add-Problem $file.Name $entry.Line "grapheme '$grapheme' of '$($entry.Word)' is not in phase $phase or an earlier phase"
                    }
                }
                $spelt = $entry.Graphemes -join ''
                if ($spelt -cne $entry.Word) {
                    Add-Problem $file.Name $entry.Line "the graphemes of '$($entry.Word)' spell '$spelt'"
                }
            }
            if ($knownWords.ContainsKey($entry.Word)) {
                Add-Problem $file.Name $entry.Line "'$($entry.Word)' is already listed ($($knownWords[$entry.Word]))"
            } else {
                $knownWords[$entry.Word] = "$($file.Name):$($entry.Line)"
                $wordCount++
            }
        }
    }
}

# --- the review gate ---------------------------------------------------------

$unreviewed = @($parsed | Where-Object { -not $_.Reviewed } | ForEach-Object { $_.Name })
if ($RequireReviewed) {
    foreach ($name in $unreviewed) {
        Add-Problem $name 0 'has no complete review header (reviewed-by and reviewed-on), so it is test data, which a release build refuses'
    }
}

# --- report ------------------------------------------------------------------

if ($problems.Count -gt 0) {
    Write-Host ''
    Write-Host 'Phonics data problem(s):' -ForegroundColor Red
    foreach ($problem in $problems) {
        $where = if ($problem.Line -gt 0) { "$($problem.File):$($problem.Line)" } else { $problem.File }
        Write-Host "  ${where}  $($problem.Message)"
        $annotation = "file=data/phonics/$($problem.File)"
        if ($problem.Line -gt 0) { $annotation += ",line=$($problem.Line)" }
        Write-Host "::error ${annotation}::$($problem.Message)"
    }
    Write-Host ''
    Write-Host ("{0} problem(s) in {1} file(s)." -f $problems.Count, $files.Count) -ForegroundColor Red
    exit 1
}

$state = if ($unreviewed.Count -eq 0) { 'all reviewed' } else { "$($unreviewed.Count) of $($files.Count) are test data (no complete review header)" }
Write-Host "OK: $($files.Count) phonics file(s), $wordCount word(s), $graphemeCount grapheme(s); $state." -ForegroundColor Green
exit 0
