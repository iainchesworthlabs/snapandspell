# SPDX-License-Identifier: GPL-3.0-or-later
# libFuzzer harnesses for what the scanner is handed (docs/fuzzing.md). Built only with
# SNS_ENABLE_FUZZING (cmake/Fuzzing.cmake); each has a seed corpus and a directory of past crashes.
foreach(name frame_reader)
    sns_add_fuzz_target(${name} sns::scan)
endforeach()
