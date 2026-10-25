// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// The house faces for the test programs that start Qt Quick (the Quick Test harness and the two
// stub shells): Archivo, IBM Plex Mono and Andika, the files the application compiles in and
// registers before its engine loads (apps/app/src/main.cpp). Without them a screen is laid out in
// whatever the machine has: a different width for every label, so the layout assertions measure the
// fallback, and on macOS Qt warns that it is replacing the missing "Archivo", which fails the run.
//
// It also makes Archivo the application's default font. Text that names no family (a Qt Quick Text,
// a Control's own label) takes the platform theme's, and the offscreen platform's is the generic
// "Sans Serif", which macOS does not have: Qt then warns 'Populating font family aliases ...
// missing font family "Sans Serif"', and a run that fails on a warning cannot pass. The Cocoa
// platform of the real application supplies a system font, so only the test programs need this.
//
// Called once the application object exists and before a screen is loaded. Reads the files from the
// source tree (assets/fonts), not from a resource, so the test programs need no resource of their
// own. The glyph check of language_test.cpp does not use it: it asks what the platform's own fonts
// cover.
//
// Test support: nothing in libs and apps link it.

namespace wjs::app::test {

// Registers the six font files and sets the default font; the later calls do nothing. Returns false
// (and warns) when a file does not load.
bool load_house_fonts();

}  // namespace wjs::app::test
