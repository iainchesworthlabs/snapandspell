// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// The house faces for the test programs that start Qt Quick (the Quick Test harness and the two
// stub shells): Archivo, IBM Plex Mono and Andika, the files the application compiles in and
// registers before its engine loads (apps/app/src/main.cpp). Without them a screen is laid out in
// whatever the machine has: a different width for every label, so the layout assertions measure the
// fallback, and on macOS Qt warns that it is replacing the missing "Archivo", which fails the run.
//
// Called once the application object exists and before a screen is loaded. Reads the files from the
// source tree (assets/fonts), not from a resource, so the test programs need no resource of their
// own. The glyph check of language_test.cpp does not use it: it asks what the platform's own fonts
// cover.
//
// Test support: nothing in libs and apps link it.

namespace wjs::app::test {

// Registers the six font files; the later calls do nothing. Returns false (and warns) when a file
// does not load.
bool load_house_fonts();

}  // namespace wjs::app::test
