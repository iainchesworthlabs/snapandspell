#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Starts the web bundle in a headless Chrome and says whether the game came up.
#
#   smoke_web.sh <bundle-directory> [--chrome <path>]
#
# The bundle is served over HTTP from a throwaway local server (a file:// page cannot fetch a
# .wasm), Chrome loads index.html with software WebGL and a virtual clock that runs a minute of
# page time, and the page's DOM is read back. The game came up if the page's loading panel is
# hidden (wjs-boot.js hides it when Qt reports the program loaded) and the page is not marked
# `failed` (what it does when the program cannot start or exits). It does not read a picture: a
# headless Chrome's screenshot is taken before a WebGL canvas has painted and comes out blank.
#
# Needs python3 (the server) and a Chrome or Chromium: --chrome, $CHROME, or the first of
# google-chrome, chromium and chromium-browser on the path. It uses no browser driver and no
# package. It shows that the program loads and draws its first screen in a real browser engine; a
# game played with clicks is not run here.
set -euo pipefail

bundle="" chrome="${CHROME:-}"
while [ $# -gt 0 ]; do
  case "$1" in
    --chrome) chrome="${2:?}"; shift 2 ;;
    -*) echo "smoke_web.sh: unknown option $1" >&2; exit 2 ;;
    *) bundle="$1"; shift ;;
  esac
done
[ -f "$bundle/index.html" ] || { echo "usage: smoke_web.sh <bundle-directory> [--chrome path]" >&2; exit 2; }
if [ -z "$chrome" ]; then
  for candidate in google-chrome chromium chromium-browser; do
    if command -v "$candidate" > /dev/null; then chrome="$candidate"; break; fi
  done
fi
[ -n "$chrome" ] || { echo "smoke_web.sh: no Chrome or Chromium found (use --chrome or set CHROME)" >&2; exit 2; }

work="$(mktemp -d)"
server_pid=""
cleanup() {
  [ -z "$server_pid" ] || kill "$server_pid" 2> /dev/null || true
  rm -rf "$work"
}
trap cleanup EXIT

# A free port, then a server that knows what a .wasm is (a browser refuses to compile one served
# as anything else).
port="$(python3 -c 'import socket; s = socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1])')"
python3 - "$bundle" "$port" > /dev/null 2>&1 << 'PY' &
import functools, http.server, sys
bundle, port = sys.argv[1], int(sys.argv[2])
class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {**http.server.SimpleHTTPRequestHandler.extensions_map,
                      ".wasm": "application/wasm", ".js": "text/javascript",
                      ".webmanifest": "application/manifest+json"}
http.server.ThreadingHTTPServer(("127.0.0.1", port), functools.partial(Handler, directory=bundle)).serve_forever()
PY
server_pid=$!
for _ in $(seq 1 50); do
  python3 -c "import socket,sys; socket.create_connection(('127.0.0.1', $port), 0.2).close()" 2> /dev/null && break
  sleep 0.1
done

url="http://127.0.0.1:$port/index.html"
flags=(--no-sandbox --disable-dev-shm-usage --use-angle=swiftshader --enable-unsafe-swiftshader
       --ignore-gpu-blocklist --window-size=1100,700 --virtual-time-budget=60000
       --user-data-dir="$work/profile")

echo "smoke: $chrome loads $url"
timeout 180 "$chrome" --headless "${flags[@]}" --dump-dom "$url" > "$work/dom.html" 2> "$work/chrome.err" \
  || { echo "smoke: Chrome did not finish" >&2; tail -20 "$work/chrome.err" >&2; exit 1; }

body="$(grep -o '<body[^>]*>' "$work/dom.html" | head -n 1 || true)"
loading="$(grep -o 'id="loading"[^>]*' "$work/dom.html" | head -n 1 || true)"
echo "smoke: $body"
echo "smoke: $loading"
if [ -z "$body" ]; then
  echo "smoke: FAIL the page has no body: Chrome did not render it" >&2
  exit 1
fi
if grep -q 'class="[^"]*failed' <<< "$body"; then
  echo "smoke: FAIL the page says the game could not start:" >&2
  grep -o 'id="status"[^<]*<[^<]*' "$work/dom.html" | head -n 3 >&2 || true
  exit 1
fi
if ! grep -q 'hidden' <<< "$loading"; then
  echo "smoke: FAIL the loading panel is still showing: the game did not come up within a minute of page time" >&2
  exit 1
fi
echo "smoke: ok, the game came up"
