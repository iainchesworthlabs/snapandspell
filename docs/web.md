# Playing in a browser

Word Jewels Solver builds to WebAssembly: the same program, drawn by Qt Quick into a page. This page says
what the web build is, what it does not have, how it keeps what a child has done, and what is needed to
put the game on a web address. How it is built is in [Building](building.md#web) and how CI checks it is in
[CI](ci.md#web).

## What it is

- **One page and four files.** `index.html`, a boot script (`wjs-boot.js`), Qt's loader (`qtloader.js`) and the
  program (`WordJewelsSolver.js` and `WordJewelsSolver.wasm`), with an icon and a manifest. It is a directory of
  static files: any web server serves it, over HTTP or HTTPS, from the root or from a folder.
- **The whole game.** Solo Play with the 2D board scene, the parent area and its settings, My jewels, the
  history, the languages, and Mode A's screens, which tell a browser has no camera and offer typing the board.
- **Nothing is sent anywhere.** The page asks for nothing from another site (a check of the bundle,
  `verify_web.sh`, fails if it does), the program has no network code, and the page's
  Content-Security-Policy tells the browser to refuse any connection but to the page's own site and to
  run no script but the bundle's own (no `eval`: the program is linked without dynamic execution for that).
  What a child has done stays in the browser.
- **A size.** The program is about 31 MB as built and about 11 MB sent by a server that compresses
  (`gzip`). It is a download on the first visit and is then in the browser's cache. The budget that CI holds
  it to is 40 MB and 14 MB: what takes most of it is Qt Quick itself, the fonts (1.7 MB) and the English word
  list (1.7 MB).

## What it does not have

| In the browser | Because |
| --- | --- |
| No sound and no spoken words | Qt's WebAssembly kit has no Qt Multimedia and no Qt Speech, so the build uses the silent variants the program has for a kit without them. A Web Audio backend and the browser's speech synthesis would each be a variant of their own. |
| No shake of the device | No Qt Sensors in the kit. The Shake button, Space and a drag are the ways in, as on a desktop. |
| No camera: Mode A reads a typed board | No Qt Multimedia. The scan screens say "No camera" and offer **Type in your board**. |
| A vibration, where the browser has one | `navigator.vibrate`: Chrome on Android. Safari, and every desktop browser, do not. |
| Hebrew, Chinese and Japanese show missing-letter boxes until their fonts are bundled | A browser has no system fonts the page may count on. The Latin faces are in the program; the Noto subsets for these scripts are the fonts work's ([Toybox restyle, 12.4](design/toybox/DESIGN.md)). A browser that grants Qt its local fonts may supply them meanwhile. |
| One thread | The multithreaded kit needs the page served with `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers, which GitHub Pages cannot send. The game has no thread of its own, so nothing is lost. A host that can send the headers could use the other kit later. |
| A window of at least 900 x 620 | The least window of the application. A smaller viewport scrolls the page instead of squeezing the layout. Phone layouts come with the design's tablet and phone work. |

## The licence it carries

The bundle has `LICENSE.txt` (the GPL) and `NOTICES.txt` (the same third-party notices the program shows
under About, Licences). Its notices differ from the desktop's in one respect: the desktop program is
GPL-3.0-or-later ([Licensing](licensing.md)), but the web bundle carries Qt's `qtloader.js`, which The Qt
Company offers under GPL-3.0 only or commercially, and `NOTICES.txt` says the bundle is conveyed under GPL
version 3 for that reason. That is the one GPL-only file in any package of this project, written by the
web build's author and not yet confirmed by the owner (a loader of the project's own would remove it).

## What it keeps, and where

The program's one file, its store (the child's words and stars and the grown-ups' settings), is written to
`/persist` in the program's own file system. `wjs-boot.js` makes `/persist` a directory of the browser's
**IndexedDB** before the program starts, reads what an earlier visit left there, and writes it back every
four seconds and when the page is hidden or closed. So:

- Progress survives a reload, closing the tab and restarting the browser, **in the same browser profile and
  on the same address** (the origin).
- It is gone when the person clears the site's data, in a private window when the window closes, and in a
  browser that refuses IndexedDB (a private window of some browsers). The game still plays then, and keeps
  nothing past the page.
- It does not move to another device or browser. Nothing here syncs.

## Putting it on an address

The build makes the directory `build/web-release/web-bundle/` and CI zips it as `WordJewelsSolver-<version>-web.zip`
(the `web-bundle` artifact of the [Web build](ci.md#web) workflow). Three ways to host it, and what each
needs:

1. **GitHub Pages, under the documentation site** (for example `…/play/`). The documentation site is built
   and published by the shared `docs.yml` and `docs-deploy.yml` of `icl-common-infra`, and a Pages site
   is one artifact: the bundle has to be placed in the site before it is uploaded. That is a change to the
   shared workflow (an input naming extra files) or to this repository's `docs.yml` wrapper, and it is the
   owner's to choose, with the repository's Pages settings, which this repository's workflows do not touch.
   Pages cannot send the headers the multithreaded kit needs, so this is the single-threaded build. Whether
   Pages compresses `.wasm` has not been checked here.
2. **Any static host** (a bucket behind a CDN, Cloudflare Pages, Netlify). Unzip the bundle and upload it. A
   host that sends `Content-Type: application/wasm` for `.wasm` (all do) and compresses (brotli does better
   than gzip on a `.wasm`) is all it needs.
3. **A computer, for trying it.** `python3 -m http.server --directory build/web-release/web-bundle` and open
   `http://localhost:8000`. A page opened from `file://` cannot fetch a `.wasm`.

Whichever host: serve the files as they are, with no rewriting, and use HTTPS for anything public (a browser
gives IndexedDB and the vibration call only to a secure page, and `localhost` counts as one).

## What has been run

On a Linux machine with Emscripten 5.0.5 and Qt 6.12.0's `wasm_singlethread` kit, the program was built, linked
and run in a headless Chromium (software WebGL): Home, Solo Play's box screen, the shake and the drop of the 2D
scene, the board, the parent gate (held for two seconds), the parent dashboard and Mode A's setup screen
("No camera", "Type in your board") drew with the bundled fonts; the store file was written under `/persist`
and was there after a reload. Not run: Safari, Firefox, any phone or tablet browser, a real GPU, touch input,
or a long game. [CI](ci.md#what-the-web-has-and-has-not-shown) says which of this a workflow repeats: the build, the
bundle's checks and the start of the page in Chrome run on every change that touches the web build.
