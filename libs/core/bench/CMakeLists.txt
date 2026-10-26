# --- Performance tests: work budgets and a few relative timings -------------
# sns_perf_tests holds the tier that fails a change that makes the solver
# meaningfully more expensive, the way a unit test fails a change that makes it
# wrong: a budget on what the solver and the loader do (visits, words, trie
# nodes), exact on every machine, and wall-time checks against a calibration
# workload that only run on request. Its own binary so that it can be built and
# run alone and so that nothing else's static state or allocator churn is in
# the way of a stopwatch. sns::lib and the scanner (with its synthetic corpus,
# libs/scan/tests/synth): no Qt and no platform layer.
# docs/testing.md, "Performance tests".
add_executable(sns_perf_tests
    corpus.cpp
    data_metrics.cpp
    data_work_test.cpp
    game_work_test.cpp
    measure.cpp
    metrics.cpp
    report_test.cpp
    timing_test.cpp
    work_test.cpp)

target_link_libraries(sns_perf_tests
    PRIVATE
        sns::lib
        sns_scan_synth
        sns::warnings
        sns::sanitizers
        sns::coverage
        Catch2::Catch2WithMain)

target_include_directories(sns_perf_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# What a build is, as far as a stopwatch goes (perf_build.hpp). Values
# CMake passes in, not preprocessor conditionals: the configuration is a
# generator expression so that a multi-config generator is right per
# configuration, and the instrumented flag is the options that put coverage or
# Tracy in (the sanitizers only ever apply to Debug, which is not an optimised
# configuration). The word list and the dice are the shipped ones, by absolute
# path.
target_compile_definitions(sns_perf_tests PRIVATE
    "SNS_PERF_OPTIMISED=$<IF:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>,1,0>"
    "SNS_PERF_INSTRUMENTED=$<IF:$<OR:$<BOOL:${SNS_ENABLE_COVERAGE}>,$<BOOL:${SNS_ENABLE_TRACY}>>,1,0>"
    "SNS_PERF_BUILD_TYPE=\"$<CONFIG>\""
    "SNS_PERF_DICTIONARY=\"${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt\""
    "SNS_PERF_DICE_DIRECTORY=\"${PROJECT_SOURCE_DIR}/data/dice\"")

# The one environment variable the tier reads, SNS_PERF_TIMING (measure.cpp),
# is read with std::getenv, which MSVC's runtime deprecates in favour of a
# call that only it has. It is a read of one variable at start-up; this says
# so to the runtime rather than to a source file.
target_compile_definitions(sns_perf_tests PRIVATE _CRT_SECURE_NO_WARNINGS)

sns_deploy_sanitizer_runtime(sns_perf_tests)

# Two registrations of one binary, because a stopwatch is the one test that
# cannot share the machine: [timing] cases are RUN_SERIAL, so `ctest -j` runs
# them alone, and they skip themselves unless SNS_PERF_TIMING is set (the
# *-perf test presets set it; docs/testing.md says why they are opt-in). The
# budget cases are plain counts and run in parallel with everything else, in
# every optimised build and skipped in the others. The hidden [.perf-report]
# cases are in neither: they are a tool, not a test. Every case carries the
# `perf` label, so `ctest -L perf` is the whole tier.
catch_discover_tests(sns_perf_tests DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS
    TEST_PREFIX "perf."
    TEST_SPEC "[perf]~[timing]")
catch_discover_tests(sns_perf_tests DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS
    TEST_PREFIX "perf."
    TEST_SPEC "[timing]"
    PROPERTIES RUN_SERIAL TRUE)
