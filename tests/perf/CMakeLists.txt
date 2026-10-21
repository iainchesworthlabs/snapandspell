# --- Performance tests: work budgets and a few relative timings -------------
# wjs_perf_tests holds the tier that fails a change that makes the solver
# meaningfully more expensive, the way a unit test fails a change that makes it
# wrong: a budget on what the solver and the loader do (visits, words, trie
# nodes), exact on every machine, and wall-time checks against a calibration
# workload that only run on request. Its own binary so that it can be built and
# run alone and so that nothing else's static state or allocator churn is in
# the way of a stopwatch. wjs::lib and the scanner (with its synthetic corpus,
# tests/scan/synth): no Qt and no platform layer.
# docs/testing.md, "Performance tests".
add_executable(wjs_perf_tests
    corpus.cpp
    data_metrics.cpp
    data_work_test.cpp
    game_work_test.cpp
    measure.cpp
    metrics.cpp
    report_test.cpp
    timing_test.cpp
    work_test.cpp)

target_link_libraries(wjs_perf_tests
    PRIVATE
        wjs::lib
        wjs_scan_synth
        wjs::warnings
        wjs::sanitizers
        wjs::coverage
        Catch2::Catch2WithMain)

target_include_directories(wjs_perf_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# What a build is, as far as a stopwatch goes (perf_build.hpp). Values
# CMake passes in, not preprocessor conditionals: the configuration is a
# generator expression so that a multi-config generator is right per
# configuration, and the instrumented flag is the options that put coverage or
# Tracy in (the sanitizers only ever apply to Debug, which is not an optimised
# configuration). The word list and the dice are the shipped ones, by absolute
# path.
target_compile_definitions(wjs_perf_tests PRIVATE
    "WJS_PERF_OPTIMISED=$<IF:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>,1,0>"
    "WJS_PERF_INSTRUMENTED=$<IF:$<OR:$<BOOL:${WJS_ENABLE_COVERAGE}>,$<BOOL:${WJS_ENABLE_TRACY}>>,1,0>"
    "WJS_PERF_BUILD_TYPE=\"$<CONFIG>\""
    "WJS_PERF_DICTIONARY=\"${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt\""
    "WJS_PERF_DICE_DIRECTORY=\"${PROJECT_SOURCE_DIR}/data/dice\"")

# The one environment variable the tier reads, WJS_PERF_TIMING (measure.cpp),
# is read with std::getenv, which MSVC's runtime deprecates in favour of a
# call that only it has. It is a read of one variable at start-up; this says
# so to the runtime rather than to a source file.
target_compile_definitions(wjs_perf_tests PRIVATE _CRT_SECURE_NO_WARNINGS)

wjs_deploy_sanitizer_runtime(wjs_perf_tests)

# Two registrations of one binary, because a stopwatch is the one test that
# cannot share the machine: [timing] cases are RUN_SERIAL, so `ctest -j` runs
# them alone, and they skip themselves unless WJS_PERF_TIMING is set (the
# *-perf test presets set it; docs/testing.md says why they are opt-in). The
# budget cases are plain counts and run in parallel with everything else, in
# every optimised build and skipped in the others. The hidden [.perf-report]
# cases are in neither: they are a tool, not a test. Every case carries the
# `perf` label, so `ctest -L perf` is the whole tier.
catch_discover_tests(wjs_perf_tests DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS
    TEST_PREFIX "perf."
    TEST_SPEC "[perf]~[timing]")
catch_discover_tests(wjs_perf_tests DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS
    TEST_PREFIX "perf."
    TEST_SPEC "[timing]"
    PROPERTIES RUN_SERIAL TRUE)
