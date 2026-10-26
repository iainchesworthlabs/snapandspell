// SPDX-License-Identifier: GPL-3.0-or-later
// Starts the program of the web build. Loaded by index.html after Qt's loader (qtloader.js) and the
// program's own script, which defines the entry function named by <body data-entry>.
//
// What this adds to Qt's loader, and why:
//   * The person's data. The program keeps one file, its store (the child's words and stars and
//     the parents' settings). A browser page has only an in-memory file system, so before the
//     program starts this mounts IndexedDB as a file system at PERSIST_DIR, loads what an earlier
//     visit left there, and saves the directory back every few seconds and when the page is hidden
//     or closed. The program is told to keep its per-user data there with XDG_DATA_HOME (Qt's
//     QStandardPaths reads it). The directory name is also in paths_web.cpp and platform.cmake;
//     tests/platform/check_web.cmake holds the three to one name.
//   * What the person sees while it loads and if it cannot (the page's #status line).
//
// If IndexedDB is not available (some private windows refuse it) the game still plays and says
// nothing is kept; the person is not asked anything.
'use strict';

(() => {
  const PERSIST_DIR = '/persist';
  const SAVE_EVERY_MS = 4000;

  const loading = document.getElementById('loading');
  const screen = document.getElementById('screen');
  const status = document.getElementById('status');

  const fail = (text) => {
    document.body.classList.add('failed');
    loading.hidden = false;
    status.textContent = text;
  };

  // Mounts and loads the persistent directory; resolves to whether it is persistent.
  const mountPersistent = async (instance) => {
    const FS = instance.FS;
    try { FS.mkdirTree(PERSIST_DIR); } catch (e) { /* it exists */ }
    try {
      FS.mount(FS.filesystems.IDBFS, {}, PERSIST_DIR);
      await new Promise((resolve, reject) =>
        FS.syncfs(true, (err) => (err ? reject(err) : resolve())));
    } catch (e) {
      console.warn('Progress cannot be kept in this browser window:', e);
      return false;
    }
    let saving = false;
    const save = () => {
      if (saving) return;
      saving = true;
      FS.syncfs(false, (err) => {
        saving = false;
        if (err) console.warn('Could not save progress:', err);
      });
    };
    setInterval(save, SAVE_EVERY_MS);
    document.addEventListener('visibilitychange', () => {
      if (document.visibilityState === 'hidden') save();
    });
    window.addEventListener('pagehide', save);
    return true;
  };

  const start = async () => {
    const entry = window[document.body.dataset.entry];
    if (typeof entry !== 'function') {
      fail('The game could not be found. Reload the page; if it still fails, the files did not all arrive.');
      return;
    }
    try {
      // Kept for a test (or a person) to look at the program's file system from the console.
      window.wjs = { instance: null, persistent: false };
      await qtLoad({
        qt: {
          // Qt's per-user data directory is $XDG_DATA_HOME/<organisation>/<application>.
          environment: { XDG_DATA_HOME: PERSIST_DIR },
          containerElements: [screen],
          onLoaded: () => { loading.hidden = true; },
          onExit: (exit) => {
            fail(exit.crashed
              ? 'The game stopped (' + (exit.text || 'an error') + '). Reload the page to play again.'
              : 'The game has closed. Reload the page to play again.');
          },
          // The wrapper runs between Emscripten making the program and Qt starting it, which is
          // the only moment the file system can be mounted and filled before main() reads it.
          entryFunction: async (emscriptenConfig) => {
            const instance = await entry(emscriptenConfig);
            window.wjs.instance = instance;
            window.wjs.persistent = await mountPersistent(instance);
            return instance;
          },
        },
      });
    } catch (e) {
      console.error(e);
      fail('The game could not start: ' + (e && e.message ? e.message : e));
    }
  };

  start();
})();
