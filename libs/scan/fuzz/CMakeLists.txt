# SPDX-License-Identifier: GPL-3.0-or-later
# libFuzzer harnesses for what the scanner is handed (docs/fuzzing.md). Built only with
# WJS_ENABLE_FUZZING (cmake/Fuzzing.cmake); each has a seed corpus and a directory of past crashes.
foreach(name frame_reader)
    wjs_add_fuzz_target(${name} wjs::scan)
endforeach()
