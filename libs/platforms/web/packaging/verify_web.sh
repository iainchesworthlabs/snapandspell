#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Looks inside a web bundle (the directory the web build makes, <build>/web-bundle) and says what
# is wrong with it, from the files alone. Run after the build and before the bundle is zipped.
#
#   verify_web.sh <bundle-directory> [--version <X.Y.Z>]
#
# What it checks:
#   - every file the page needs is there, and the page names only files that are;
#   - the page and its scripts fetch nothing from another site (nothing is requested but the
#     bundle's own files: no font, script or analytics from a third party), and the page says so
#     to the browser with a Content-Security-Policy that allows no `eval` and no connection but
#     to its own site;
#   - the program is a WebAssembly module, built for one thread (no SharedArrayBuffer, which a
#     page can use only if it is served with cross-origin isolation headers that GitHub Pages
#     cannot send), and without Qt Quick 3D, Qt Multimedia or Qt Test linked in;
#   - the program is inside its size budget: the first load is a download, and a child waits for it
#     (docs/ci.md, "Web");
#   - the manifest is JSON and names the icons it ships;
#   - with --version, the program carries that version.
set -euo pipefail

# The budget, in bytes: the .wasm as built, and gzipped (what a server that compresses sends).
# At the time of writing the bundle is about 32 MB and 11 MB. A rise past these is a decision, not
# a surprise: Quick 3D alone was 10 MB of it before the web build dropped it.
max_wasm=$((40 * 1024 * 1024))
max_wasm_gzip=$((14 * 1024 * 1024))

bundle="" version=""
while [ $# -gt 0 ]; do
  case "$1" in
    --version) version="${2:?}"; shift 2 ;;
    -*) echo "verify_web.sh: unknown option $1" >&2; exit 2 ;;
    *) bundle="$1"; shift ;;
  esac
done
[ -d "$bundle" ] || { echo "usage: verify_web.sh <bundle-directory> [--version X.Y.Z]" >&2; exit 2; }

failures=0
checks=0
ok() { checks=$((checks + 1)); echo "  ok    $1"; }
bad() { checks=$((checks + 1)); failures=$((failures + 1)); echo "  FAIL  $1" >&2; }

echo "web bundle: $bundle"

for file in index.html wjs-boot.js qtloader.js WordJewelsSolver.js WordJewelsSolver.wasm \
            manifest.webmanifest icon-64.png icon-256.png LICENSE.txt NOTICES.txt; do
  [ -s "$bundle/$file" ] && ok "$file is there" || bad "$file is missing or empty"
done

# The page names its scripts, its manifest and its icons, and each exists.
for ref in $(grep -oE '(src|href)="[^"#]+"' "$bundle/index.html" | sed -E 's/^(src|href)="//; s/"$//'); do
  case "$ref" in
    http*|//*) bad "index.html names another site: $ref" ;;
    *) [ -e "$bundle/$ref" ] && ok "index.html names $ref, which is there" || bad "index.html names $ref, which is not there" ;;
  esac
done

# Nothing in what the bundle writes itself reaches another site. (The program's own .js is
# Emscripten's, and carries documentation URLs in comments; it is not read for this.)
for file in index.html wjs-boot.js manifest.webmanifest; do
  if grep -E "https?://" "$bundle/$file" | grep -vE 'www\.w3\.org' | grep -q .; then
    bad "$file contains a URL"
  else
    ok "$file fetches nothing from another site"
  fi
done

# The page's Content-Security-Policy: its own files only, and neither `unsafe-eval` nor `unsafe-inline`
# for scripts (the one inline <style> is allowed for styles only).
csp="$(grep -o 'Content-Security-Policy" content="[^"]*"' "$bundle/index.html" | head -n 1 || true)"
if [ -z "$csp" ]; then
  bad "index.html has no Content-Security-Policy"
else
  ok "index.html has a Content-Security-Policy"
  grep -q "default-src 'none'" <<< "$csp" && ok "the policy denies everything it does not allow" || bad "the policy does not start from default-src 'none'"
  grep -q "connect-src 'self'" <<< "$csp" && ok "the policy lets the page connect to its own site only" || bad "the policy does not say connect-src 'self'"
  script_src="$(grep -o "script-src [^;]*" <<< "$csp" | head -n 1 || true)"
  if grep -qE "'unsafe-eval'|'unsafe-inline'|https?:" <<< "$script_src"; then bad "the policy's script-src allows eval, inline script or another site: $script_src"; else ok "the policy's script-src is 'self' and WebAssembly: $script_src"; fi
fi

# The notices say what this bundle is: no Quick 3D, and the licence of the loader script.
if [ -s "$bundle/NOTICES.txt" ]; then
  if grep -q 'Quick 3D' "$bundle/NOTICES.txt" && ! grep -q 'does not link Qt Quick 3D' "$bundle/NOTICES.txt"; then
    bad "NOTICES.txt describes a program that links Quick 3D"
  else
    ok "NOTICES.txt is the web build's (it does not claim Quick 3D)"
  fi
  grep -q 'qtloader.js' "$bundle/NOTICES.txt" && ok "NOTICES.txt names qtloader.js and its licence" || bad "NOTICES.txt does not name qtloader.js"
fi

# WebAssembly: the magic number, then single-thread and without the modules left out on purpose.
wasm="$bundle/WordJewelsSolver.wasm"
if [ -s "$wasm" ]; then
  [ "$(head -c 4 "$wasm" | od -An -tx1 | tr -d ' \n')" = "0061736d" ] && ok "the program is a WebAssembly module" || bad "the program is not a WebAssembly module"

  if grep -q SharedArrayBuffer "$bundle/WordJewelsSolver.js"; then
    bad "the program uses SharedArrayBuffer: it needs cross-origin isolation, which GitHub Pages cannot give"
  else
    ok "the program is single-threaded (no SharedArrayBuffer)"
  fi

  for forbidden in QQuick3DNode QMediaPlayer QtTest QTest; do
    if grep -qa "$forbidden" "$wasm"; then
      bad "the program carries $forbidden, which the web build leaves out"
    else
      ok "the program carries no $forbidden"
    fi
  done

  size=$(wc -c < "$wasm")
  gz=$(gzip -9 -c "$wasm" | wc -c)
  [ "$size" -le "$max_wasm" ] && ok "the program is $((size / 1048576)) MB (budget $((max_wasm / 1048576)) MB)" || bad "the program is $((size / 1048576)) MB, over the $((max_wasm / 1048576)) MB budget"
  [ "$gz" -le "$max_wasm_gzip" ] && ok "gzipped it is $((gz / 1048576)) MB (budget $((max_wasm_gzip / 1048576)) MB)" || bad "gzipped it is $((gz / 1048576)) MB, over the $((max_wasm_gzip / 1048576)) MB budget"

  if [ -n "$version" ]; then
    grep -qa "$version" "$wasm" && ok "the program carries version $version" || bad "the program does not carry version $version"
  fi
fi

# The manifest is JSON and its icons are the bundle's.
if python3 -c 'import json,sys; m=json.load(open(sys.argv[1])); assert m["name"]; assert m["icons"]' "$bundle/manifest.webmanifest" 2>/dev/null; then
  ok "manifest.webmanifest is JSON with a name and icons"
  for icon in $(python3 -c 'import json,sys; print(" ".join(i["src"] for i in json.load(open(sys.argv[1]))["icons"]))' "$bundle/manifest.webmanifest"); do
    [ -e "$bundle/$icon" ] && ok "the manifest's $icon is there" || bad "the manifest names $icon, which is not there"
  done
else
  bad "manifest.webmanifest is not JSON with a name and icons"
fi

echo
if [ "$failures" -eq 0 ]; then
  echo "web bundle: $checks check(s) passed"
else
  echo "web bundle: $failures of $checks check(s) FAILED" >&2
  exit 1
fi
