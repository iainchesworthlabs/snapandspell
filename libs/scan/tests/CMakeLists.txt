# The scanner's tests: its own binary, so that the synthetic corpus and its
# renderer stay out of wjs_unit_tests, and the scanner can be built and run alone.
# docs/design/scanner.md and docs/testing.md ("The scanner").

# The corpus: frames of a board of dice drawn from numbers and a seed, and the
# counting that holds the reader to its target. A test tool, never linked into
# the application.
add_library(wjs_scan_synth STATIC
    synth/evaluate.cpp
    synth/png.cpp
    synth/scene.cpp)
target_include_directories(wjs_scan_synth PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/synth")
target_link_libraries(wjs_scan_synth
    PUBLIC
        wjs::scan
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

add_executable(wjs_scan_tests
    scan_confirmation_test.cpp
    scan_corpus_test.cpp
    scan_finder_test.cpp
    scan_geometry_test.cpp
    scan_glyph_test.cpp
    scan_image_test.cpp
    scan_platform_test.cpp
    scan_reader_test.cpp)

target_link_libraries(wjs_scan_tests
    PRIVATE
        wjs_scan_synth
        wjs::scan
        wjs::lib
        wjs::platform
        wjs::warnings
        wjs::sanitizers
        wjs::coverage
        Catch2::Catch2WithMain)

# Whether this build is optimised, as a value CMake passes in (the corpus is under
# a few seconds in an optimised build and much longer in Debug with the
# sanitizers, so the target is held there and a smoke subset runs everywhere).
target_compile_definitions(wjs_scan_tests PRIVATE
    "WJS_SCAN_OPTIMISED=$<IF:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>,1,0>"
    "WJS_SCAN_BUILD_TYPE=\"$<CONFIG>\"")

# The recorded-frames case reads one environment variable at start with
# std::getenv, which MSVC's runtime deprecates in favour of a call only it has (the
# performance tier does the same, tests/CMakeLists.txt).
target_compile_definitions(wjs_scan_tests PRIVATE _CRT_SECURE_NO_WARNINGS)

wjs_deploy_sanitizer_runtime(wjs_scan_tests)

catch_discover_tests(wjs_scan_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS)

# Writes the corpus to a directory, to look at (tools/scan/render_corpus.cpp).
# A tool rather than a test, so it is not part of the default build: ask for it
# with `cmake --build <build> --target wjs_scan_render` when its pictures are
# wanted (docs/testing.md, "The scanner").
add_executable(wjs_scan_render "${PROJECT_SOURCE_DIR}/tools/scan/render_corpus.cpp")
set_target_properties(wjs_scan_render PROPERTIES EXCLUDE_FROM_ALL ON)
target_link_libraries(wjs_scan_render
    PRIVATE
        wjs_scan_synth
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
wjs_deploy_sanitizer_runtime(wjs_scan_render)

# The scanner does not write a frame to disk, capture audio or use the network: a
# scan of its sources and of the camera and board_reader files, and a test that the
# scan catches a file that does (libs/scan/tests/check_privacy.cmake). Both only read
# source, so they run on every host and in every CI leg.
add_test(NAME scan.privacy
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_privacy.cmake")
# The check's own test: it has to fail each fixture for the rule that fixture breaks, and
# for no other, pass the clean one, and refuse a root with nothing in it. A single
# WILL_FAIL on the check would pass on any failure, a syntax error in the script included.
add_test(NAME scan.privacy.catches_a_leak
    COMMAND "${CMAKE_COMMAND}"
        "-DCHECK=${CMAKE_CURRENT_SOURCE_DIR}/check_privacy.cmake"
        "-DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/privacy_fixture"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_privacy_selftest.cmake")
set_tests_properties(scan.privacy scan.privacy.catches_a_leak
    PROPERTIES LABELS "quality")
