# SPDX-License-Identifier: GPL-3.0-or-later
# libFuzzer harnesses for what this library reads from a file (docs/fuzzing.md). Built only with
# SNS_ENABLE_FUZZING (cmake/Fuzzing.cmake); each has a seed corpus and a directory of past crashes.
foreach(name board dictionary dice_set phase_graphemes phase_words store)
    sns_add_fuzz_target(${name} sns::lib)
endforeach()
