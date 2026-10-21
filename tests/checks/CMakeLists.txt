# ---------------------------------------------------------------------------
# The quality tier: checks about the tree, the build and the shipped data
# rather than a program's behaviour. They run on every host and in every CI leg
# (several are repeated as their own CI jobs), and none needs a display. The
# label is what `ctest -L quality` selects; the tiers proper are `unit`,
# `integration`, `system` and `ui` (docs/testing.md, "The tiers").
#
# Their scripts live in the topic directories beside this one (tests/notices,
# tests/privacy, ...), by path from WJS_TESTS_DIR; the registrations are here so
# that the whole tier, and the one list of what carries the label, is in one
# file. (app.scene_contract and app.qml_lint are the same tier, registered in
# tests/app because they read what that build generates.)
# ---------------------------------------------------------------------------

# The third-party notices (cmake/Notices.cmake): every section a package must
# state, and no placeholder left unfilled. Runs on every host; a build with the
# application names Qt and Quick 3D, and a command-line package does not claim
# to carry either.
add_test(NAME notices.sections
    COMMAND "${CMAKE_COMMAND}"
        "-DNOTICES=${WJS_NOTICES_FILE}"
        "-DVERSION=${PROJECT_VERSION}"
        "-DEXPECT_CLI=${WJS_BUILD_CLI}"
        "-DEXPECT_QT=${WJS_BUILD_APP}"
        "-DQT_VERSION=${Qt6_VERSION}"
        -P "${WJS_TESTS_DIR}/notices/check_notices.cmake")

# The CPack configuration and the install rules it packages: both programs this
# build makes, the files they read, and the notices. It reads the generated
# CPackConfig.cmake and cmake_install.cmake scripts, so a dropped install() rule
# is a failed test rather than a package missing a file (docs/building.md,
# "Packaging"). Cheap, so it runs in every ctest.
add_test(NAME package.config
    COMMAND "${CMAKE_COMMAND}"
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
        "-DVERSION=${PROJECT_VERSION}"
        "-DPRERELEASE=${WJS_PACKAGE_PRERELEASE}"
        "-DEXPECT_TRACY=${WJS_ENABLE_TRACY}"
        "-DEXPECT_CLI=${WJS_BUILD_CLI}"
        "-DEXPECT_APP=${WJS_BUILD_APP}"
        "-DEXPECT_QT_DEPLOY=${WJS_PLATFORM_QT_DEPLOY_AT_INSTALL}"
        "-DEXPECT_MULTIMEDIA=${WJS_QT_HAS_MULTIMEDIA}"
        "-DNOTICES=${WJS_NOTICES_FILE}"
        -P "${WJS_TESTS_DIR}/package/check_package.cmake")

# Every platform fragment satisfies the platform contract, on whichever host
# this runs (a build only includes the one fragment for its own platform).
add_test(NAME platform.fragments
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/platform/check_fragments.cmake")

# The Android target, held together: the manifest asks for the camera and nothing
# else (Qt's core module would add network and storage permissions at its
# template's placeholder), the presets that build it exist as the iOS one does, the
# CI job and the release job install the kits docs/ci.md and the pins file name, and
# the release job reads only the four Android signing secrets. It reads files only,
# so it runs on every host; the build itself and the package it makes are
# .github/workflows/android-build.yml's, and what runs on a device is nobody's
# yet. The manifest rules are held to a table of cases on throw-away manifests.
# The tests are named for the package, not the platform: the isolation check keeps a
# CMake file outside the platform section from naming one.
add_test(NAME platform.apk
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/platform/check_android.cmake")
add_test(NAME platform.apk.selftest
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/platform-apk"
        -P "${WJS_TESTS_DIR}/platform/test_check_android.cmake")

# The Tracy policy: Debug builds are instrumented by default, and nothing that
# ships has it - the *-debug and *-profiling presets turn the option on, the
# release and coverage presets keep it off, and the manifest feature that
# installs it is neither a base dependency nor a default feature
# (docs/profiling.md). Runs on every host and in every CI leg, since it only
# reads the presets.
add_test(NAME profiling.shipped_presets
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/profiling/check_presets.cmake")

# Every zone is named <layer>.<thing> and once: one name is one source location,
# and the zones a trace of the application's work is made of exist (the check
# pins the required set, so removed instrumentation is a failed test).
add_test(NAME profiling.zone_names
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/profiling/check_zone_names.cmake")

# Tracy is built for any machine of its architecture: the repository's tracy port
# (an overlay on the fork's) passes NO_ISA_EXTENSIONS, since a vcpkg cache built on
# one hosted runner is restored on another and a -march=native tracy-capture dies
# there with SIGILL. Reads files only, so it runs in every leg.
add_test(NAME profiling.tracy_portable
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/profiling/check_tracy_port.cmake")

# A trace of the program holds the names the documents say it does: the program
# is run with the Tracy capture attached and the exported trace is read
# (check_trace.cmake). check_zone_names reads the sources, which cannot tell a zone
# in code that never runs, or a name chosen at run time (the scene's stage zones),
# from one a trace has. Only a build with Tracy on has a client to capture, and the
# capture tools are built with it (vcpkg's `profiling` feature, in
# vcpkg_installed/<triplet>/tools/tracy); in any other build the cases report
# themselves skipped (exit 4) with the reason. A port of its own for each, since
# every Tracy client listens on 8086 and a capture attaches to whichever program
# started first (docs/profiling.md, "Which program the capture attaches to").
set(WJS_TRACY_TOOLS_DIR "")
if(WJS_ENABLE_TRACY)
    set(WJS_TRACY_TOOLS_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/tracy")
endif()
set(WJS_TRACE_TESTS "")
if(WJS_BUILD_CLI)
    add_test(NAME profiling.trace_cli
        COMMAND "${CMAKE_COMMAND}"
            -DCASE=cli
            "-DTRACY_TOOLS=${WJS_TRACY_TOOLS_DIR}"
            "-DPROGRAM=$<TARGET_FILE:wjs-cli>"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/profiling"
            -DPORT=18086
            "-DEXE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
            -P "${WJS_TESTS_DIR}/profiling/check_trace.cmake")
    list(APPEND WJS_TRACE_TESTS profiling.trace_cli)
endif()
if(WJS_BUILD_APP)
    add_test(NAME profiling.trace_app
        COMMAND "${CMAKE_COMMAND}"
            -DCASE=app
            "-DTRACY_TOOLS=${WJS_TRACY_TOOLS_DIR}"
            "-DPROGRAM=$<TARGET_FILE:WordJewelsSolver>"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/profiling"
            -DPORT=18087
            "-DEXE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
            -P "${WJS_TESTS_DIR}/profiling/check_trace.cmake")
    list(APPEND WJS_TRACE_TESTS profiling.trace_app)
endif()
if(WJS_TRACE_TESTS)
    # Alone: a capture needs the machine's attention for the seconds it takes to
    # find the program, and the two use ports of their own but share a disk.
    set_tests_properties(${WJS_TRACE_TESTS} PROPERTIES
        SKIP_RETURN_CODE 4
        RUN_SERIAL TRUE
        TIMEOUT 240
        LABELS "system;profiling")
endif()

# A leg that holds the performance tier's work budgets ran them. Every work case
# stands aside (SKIP) in a build that is not optimised, and a skip is not a
# failure, so a leg of Debug builds passes with the whole tier skipped. This runs
# the work cases and fails when an optimised build (or a workflow that sets
# WJS_REQUIRE_PERF_BUDGETS=1) ran none; in any other build it reports itself
# skipped (exit 4) with the reason, so it is never a pass that checked nothing.
add_test(NAME profiling.perf_budgets_ran
    COMMAND "${CMAKE_COMMAND}"
        "-DPERF_TESTS=$<TARGET_FILE:wjs_perf_tests>"
        "-DEXPECT_RUN=$<IF:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>,1,0>"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/profiling"
        -P "${WJS_TESTS_DIR}/profiling/check_perf_ran.cmake")
set_tests_properties(profiling.perf_budgets_ran PROPERTIES
    SKIP_RETURN_CODE 4
    TIMEOUT 300)

# The workflow's toolchain pins (CMake, Ninja, Qt, aqtinstall) are the ones in
# .github/toolchain-versions.json, and the vcpkg baseline is the pinned commit.
# Runs on every host: it only reads the files.
add_test(NAME toolchain.versions
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/toolchain/check_versions.cmake")

# The version is one number: project(VERSION), vcpkg.json and the newest
# CHANGELOG.md release agree (and the release job runs the same script with the
# tag, tests/toolchain/check_version.cmake). The check held to a table of cases
# on throw-away trees.
add_test(NAME toolchain.version
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DVERSION=${PROJECT_VERSION}"
        -P "${WJS_TESTS_DIR}/toolchain/check_version.cmake")
add_test(NAME toolchain.version.selftest
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/toolchain-version"
        -P "${WJS_TESTS_DIR}/toolchain/test_check_version.cmake")

# The nightly analysis integration: the presets the workflow names exist with
# their settings, the workflow is scheduled and stays out of merge_group, and
# CodeQL is nightly rather than per-push. Runs on every host; it only reads the
# files (docs/ci.md, "Nightly analysis").
add_test(NAME analysis.config
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/analysis/check_analysis.cmake")

# The privacy rules of the first release that the source can say whether they
# hold (docs/design/application.md, "Privacy, built in and checked"): no audio
# captured, no network used, nothing exported, one file written and only by the
# store. Runs on every host and in every CI leg, since it only reads the tree.
add_test(NAME privacy.sources
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/privacy/check_sources.cmake")

# The scan held to a table of cases on throw-away trees, so that a pattern that
# stopped matching is a failure and not a pass.
add_test(NAME privacy.sources.selftest
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/privacy"
        -P "${WJS_TESTS_DIR}/privacy/test_check_sources.cmake")

# The sentences of the parent area are in one file and are the design's, with
# none of the words an assessment, a grade or a gap is made of
# (docs/design/dashboard.md, "Wording"), and the check held to cases of its own.
add_test(NAME parent.wording
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${WJS_TESTS_DIR}/parent/check_wording.cmake")
add_test(NAME parent.wording.selftest
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/parent-wording"
        -P "${WJS_TESTS_DIR}/parent/test_check_wording.cmake")

set(WJS_QUALITY_TESTS
    notices.sections
    package.config
    platform.fragments
    platform.apk
    platform.apk.selftest
    profiling.perf_budgets_ran
    profiling.shipped_presets
    profiling.tracy_portable
    profiling.zone_names
    toolchain.versions
    toolchain.version
    toolchain.version.selftest
    analysis.config
    privacy.sources
    privacy.sources.selftest
    parent.wording
    parent.wording.selftest)

if(WJS_BUILD_APP)
    # The catalogues in i18n/ are what lupdate makes from the QML sources now:
    # a qsTr() added (or a line moved) without regenerating them is a failure.
    # It needs the kit's lupdate, so it reports itself skipped (exit 4) when Qt's
    # Linguist tools were not found (docs/ci.md, "The interface's languages").
    set(WJS_LUPDATE_PATH "")
    if(TARGET Qt6::lupdate)
        set(WJS_LUPDATE_PATH "$<TARGET_FILE:Qt6::lupdate>")
    endif()
    add_test(NAME i18n.catalogues
        COMMAND "${CMAKE_COMMAND}"
            "-DLUPDATE=${WJS_LUPDATE_PATH}"
            "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/i18n-drift"
            -P "${WJS_TESTS_DIR}/i18n/check_catalogues.cmake")
    set_tests_properties(i18n.catalogues PROPERTIES SKIP_RETURN_CODE 4)
    list(APPEND WJS_QUALITY_TESTS i18n.catalogues)
endif()

# The release build's phonics gate (tools/data/check_phonics_review.cmake): a
# CMake script, so it runs on a host without PowerShell. Held to a table of
# cases on throw-away data, and, with the gate on (a release configuration), to
# the data there is.
add_test(NAME data.phonics.review_gate.selftest
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/phonics-review"
        -P "${PROJECT_SOURCE_DIR}/tools/data/test_check_phonics_review.cmake")
list(APPEND WJS_QUALITY_TESTS data.phonics.review_gate.selftest)
if(WJS_REQUIRE_REVIEWED_PHONICS)
    add_test(NAME data.phonics.review_gate
        COMMAND "${CMAKE_COMMAND}"
            "-DROOT=${PROJECT_SOURCE_DIR}"
            -P "${PROJECT_SOURCE_DIR}/tools/data/check_phonics_review.cmake")
    list(APPEND WJS_QUALITY_TESTS data.phonics.review_gate)
endif()

# The script checks that run a PowerShell tool over the tree. No preprocessor
# conditionals or defines, no OS headers outside the platform section, no OS
# tests in CMake outside the selector: CI runs the isolation check as its own
# job too; this is the same check, so a developer sees it fail locally.
find_program(WJS_PWSH NAMES pwsh)
if(WJS_PWSH)
    # The word lists: lowercase a-z, sorted, unique, and every category and
    # tier word is in the dictionary.
    # The project's dice are what tools/data/make_dice.ps1 makes from the
    # published Scrabble-style letter table: the method and the dice both have
    # to match.
    add_test(NAME data.dice
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/data/make_dice.ps1"
            -Root "${PROJECT_SOURCE_DIR}" -Check)

    add_test(NAME data.wordlists
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/data/check_wordlists.ps1"
            -Root "${PROJECT_SOURCE_DIR}")

    # The feedback sounds are what tools/sounds/make_sounds.ps1 writes: the
    # placeholders are committed, and a hand-edited one (or a name the script
    # does not know) is a failed test rather than a game with a missing cue.
    add_test(NAME data.sounds
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/sounds/make_sounds.ps1"
            -Root "${PROJECT_SOURCE_DIR}" -Check)

    # The phonics data under data/phonics, and the checker that reads it. There
    # is none yet: the files have to be written and reviewed by a person
    # qualified to do it. With the review gate on (a release configuration),
    # a file without its review header fails here as well as in the build.
    set(WJS_PHONICS_CHECK_ARGS "")
    if(WJS_REQUIRE_REVIEWED_PHONICS)
        list(APPEND WJS_PHONICS_CHECK_ARGS -RequireReviewed)
    endif()
    add_test(NAME data.phonics
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/data/check_phonics.ps1"
            -Root "${PROJECT_SOURCE_DIR}" ${WJS_PHONICS_CHECK_ARGS})

    # The checker held to a table of cases on throw-away data: the review gate
    # must refuse what it should, so the gate itself is tested.
    add_test(NAME data.phonics.checker
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/data/test_check_phonics.ps1")

    add_test(NAME platform.isolation
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/checks/check_platform_isolation.ps1"
            -Root "${PROJECT_SOURCE_DIR}")

    # The release tooling (tools/release): the tag checks, signing and
    # verification with a throw-away key, and the SBOM. It needs bash, gpg, jq,
    # git and pwsh; without them it skips (exit 77) rather than fails, and
    # release-tooling.yml runs it on a runner that has them (docs/ci.md, "Releases").
    add_test(NAME release.tooling
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${WJS_TESTS_DIR}/package/run_release_tools_test.ps1")
    set_tests_properties(release.tooling PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)

    list(APPEND WJS_QUALITY_TESTS
        data.dice data.wordlists data.sounds data.phonics data.phonics.checker
        platform.isolation release.tooling)
endif()

set_tests_properties(${WJS_QUALITY_TESTS} PROPERTIES LABELS "quality")
