find_package(Catch2 3 CONFIG REQUIRED)
include(Catch)

add_executable(wjs_unit_tests
    unit/board_test.cpp
    unit/calendar_test.cpp
    unit/dashboard_test.cpp
    unit/dice_data_test.cpp
    unit/dice_sets_test.cpp
    unit/dice_test.cpp
    unit/dictionary_test.cpp
    unit/deal_test.cpp
    unit/game_rules_test.cpp
    unit/game_session_test.cpp
    unit/grapheme_binder_test.cpp
    unit/parent_gate_test.cpp
    unit/path_test.cpp
    unit/paths_test.cpp
    unit/phase_data_test.cpp
    unit/profiling_test.cpp
    unit/scoring_test.cpp
    unit/settings_test.cpp
    unit/shake_test.cpp
    unit/seeding_test.cpp
    unit/session_store_test.cpp
    unit/solver_test.cpp
    unit/store_test.cpp)

# Whether the profiling calls are meant to have Tracy behind them in this
# build: the option CMake was given, passed in as a value (profiling_test.cpp).
target_compile_definitions(wjs_unit_tests PRIVATE
    "WJS_TEST_EXPECT_PROFILING=$<BOOL:${WJS_ENABLE_TRACY}>")

target_link_libraries(wjs_unit_tests
    PRIVATE
        wjs::lib
        wjs::platform
        wjs::warnings
        wjs::sanitizers
        wjs::coverage
        Catch2::Catch2WithMain)

# The shipped data, by its path in this checkout, for the cases about the data
# itself (dice_data_test.cpp), as the performance tier does for the dictionary.
target_compile_definitions(wjs_unit_tests PRIVATE
    "WJS_DATA_DIRECTORY=\"${PROJECT_SOURCE_DIR}/data\"")

wjs_deploy_sanitizer_runtime(wjs_unit_tests)

catch_discover_tests(wjs_unit_tests)

# The scanner: its library, the camera and board_reader interfaces, and the
# synthetic corpus that holds it to its accuracy target.
add_subdirectory(scan)

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
    perf/corpus.cpp
    perf/game_work_test.cpp
    perf/measure.cpp
    perf/metrics.cpp
    perf/report_test.cpp
    perf/timing_test.cpp
    perf/work_test.cpp)

target_link_libraries(wjs_perf_tests
    PRIVATE
        wjs::lib
        wjs_scan_synth
        wjs::warnings
        wjs::sanitizers
        wjs::coverage
        Catch2::Catch2WithMain)

target_include_directories(wjs_perf_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/perf")

# What a build is, as far as a stopwatch goes (perf/perf_build.hpp). Values
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
    TEST_SPEC "[perf]~[timing]")
catch_discover_tests(wjs_perf_tests DISCOVERY_MODE PRE_TEST ADD_TAGS_AS_LABELS
    TEST_SPEC "[timing]"
    PROPERTIES RUN_SERIAL TRUE)

# The application's model, built from its source so the test does not need the
# QML module (QML_ELEMENT registration is inert without it). Qt is only here
# when the application is.
if(WJS_BUILD_APP)
    add_executable(wjs_app_tests
        app/assist_game_test.cpp
        app/board_layout_test.cpp
        app/fake_solo_game_test.cpp
        app/language_test.cpp
        app/parent_controller_test.cpp
        app/parent_ui/demo_data.cpp
        app/path_tracer_test.cpp
        app/real_solo_game_test.cpp
        app/scan_model_test.cpp
        app/solo_model_test.cpp
        app/solver_model_test.cpp
        app/system_colour_scheme_test.cpp
        app/system_motion_test.cpp
        "${PROJECT_SOURCE_DIR}/src/app/parent/parent_controller.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/scan/assist_game.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/scan/assist_game_factory.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/scan/assist_model.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/scan/scan_model.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/board_layout.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/fake_solo_game.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/real_solo_game.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/solo_game.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/solo_game_factory.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/solo_game_setup.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/solo_layout.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/solo_model.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solver_model.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/system_colour_scheme.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/system_motion.cpp")
    set_target_properties(wjs_app_tests PROPERTIES AUTOMOC ON)
    target_include_directories(wjs_app_tests PRIVATE
        "${PROJECT_SOURCE_DIR}/src/app"
        "${PROJECT_SOURCE_DIR}/src/app/parent"
        "${PROJECT_SOURCE_DIR}/src/app/scan"
        "${PROJECT_SOURCE_DIR}/src/app/solo"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/parent_ui")
    # The shipped data, by its path in this checkout, for the case that plays a
    # standard board over the real dice and dictionary (real_solo_game_test.cpp),
    # and the catalogues the interface's languages are read from
    # (language_test.cpp).
    target_compile_definitions(wjs_app_tests PRIVATE
        "WJS_DATA_DIRECTORY=\"${PROJECT_SOURCE_DIR}/data\""
        "WJS_I18N_DIRECTORY=\"${PROJECT_SOURCE_DIR}/i18n\"")
    target_link_libraries(wjs_app_tests
        PRIVATE
            wjs_trace
            wjs_i18n
            wjs::lib
            wjs::platform
            Qt6::Core
            Qt6::Gui
            Qt6::Qml
            wjs::warnings
            wjs::sanitizers
            wjs::coverage
            Catch2::Catch2WithMain)
    wjs_deploy_sanitizer_runtime(wjs_app_tests)
    # Qt's DLLs have to be beside the binary before catch_discover_tests runs it.
    wjs_platform_deploy_qt_runtime(wjs_app_tests "")
    catch_discover_tests(wjs_app_tests)

    # The 3D board scene's script (src/app/qml/scene/ShakeScript.js) in Qt's own
    # JavaScript engine, so what is tested is the file the scene plays and not a
    # copy, with the dice simulation (DiceSim) it drives. The checks are in
    # shake_script_check.js. They are about the dice's faces: that a die lands with
    # the face the library's roll says is up, shows other faces on the way, and that
    # none of it moves a die before it is sent home. Built with the application,
    # whose scene the script is.
    add_executable(wjs_shake_script_tests
        app/shake_script_test.cpp
        "${PROJECT_SOURCE_DIR}/src/app/scene/dice_sim.cpp")
    set_target_properties(wjs_shake_script_tests PROPERTIES AUTOMOC ON)
    target_include_directories(wjs_shake_script_tests PRIVATE
        "${PROJECT_SOURCE_DIR}/src/app/scene"
        # budgets.hpp: the scene's work budgets are in the tier's one file.
        "${CMAKE_CURRENT_SOURCE_DIR}/perf")
    target_compile_definitions(wjs_shake_script_tests PRIVATE
        "WJS_SHAKE_SCRIPT=\"${PROJECT_SOURCE_DIR}/src/app/qml/scene/ShakeScript.js\""
        "WJS_SHAKE_SCRIPT_CHECK=\"${CMAKE_CURRENT_SOURCE_DIR}/app/shake_script_check.js\"")
    target_link_libraries(wjs_shake_script_tests
        PRIVATE
            Qt6::Core
            Qt6::Qml
            wjs_dice_world
            # DiceSim's scene.step zone resolves to wjs/profiling.hpp.
            wjs::profiling
            wjs::warnings
            wjs::sanitizers
            wjs::coverage
            Catch2::Catch2WithMain)
    wjs_deploy_sanitizer_runtime(wjs_shake_script_tests)
    wjs_platform_deploy_qt_runtime(wjs_shake_script_tests "")
    # Every tag is a CTest label, so the scene's work budget (tagged perf) is
    # part of `ctest -L perf` as the tier's own cases are.
    catch_discover_tests(wjs_shake_script_tests ADD_TAGS_AS_LABELS)

    # The window's frame budget (tests/app/frame_timing_test.cpp): the
    # application's own smoke run in a child process, and the report it prints.
    # Its own binary because it makes a QCoreApplication for QProcess and must
    # not share one with the model tests. It skips itself unless a clock reading
    # is about this build (an optimised, uninstrumented one) and somebody asked
    # for it with WJS_PERF_FRAME_TIMING=1: a shared runner has no display and
    # never sets it. RUN_SERIAL, because a frame budget is a stopwatch.
    add_executable(wjs_frame_timing_tests app/frame_timing_test.cpp)
    target_include_directories(wjs_frame_timing_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/perf")
    target_compile_definitions(wjs_frame_timing_tests PRIVATE
        "WJS_APP_PATH=\"$<TARGET_FILE:wjs_app>\""
        "WJS_TEST_OPTIMISED=$<IF:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>,1,0>"
        "WJS_TEST_INSTRUMENTED=$<IF:$<OR:$<BOOL:${WJS_ENABLE_COVERAGE}>,$<BOOL:${WJS_ENABLE_TRACY}>>,1,0>")
    target_link_libraries(wjs_frame_timing_tests
        PRIVATE
            Qt6::Core
            wjs::warnings
            wjs::sanitizers
            wjs::coverage
            Catch2::Catch2WithMain)
    wjs_deploy_sanitizer_runtime(wjs_frame_timing_tests)
    wjs_platform_deploy_qt_runtime(wjs_frame_timing_tests "")
    catch_discover_tests(wjs_frame_timing_tests DISCOVERY_MODE PRE_TEST
        PROPERTIES RUN_SERIAL TRUE TIMEOUT 240)

    # The parent area's screens in a stub shell (app/parent_ui): a program to
    # look at them with, and, with --check, a test that walks the gate, the
    # dashboard, the settings and the about page without a display, checks what
    # they say, and fails on any warning from QML. Not part of the application.
    qt_policy(SET QTP0001 NEW)
    qt_add_executable(wjs_parent_preview
        app/parent_ui/demo_data.cpp
        app/parent_ui/preview_main.cpp)
    set_target_properties(wjs_parent_preview PROPERTIES AUTOMOC ON)
    set_source_files_properties(app/parent_ui/PreviewHost.qml PROPERTIES QT_RESOURCE_ALIAS PreviewHost.qml)
    qt_add_qml_module(wjs_parent_preview
        URI ParentPreview
        VERSION 1.0
        QML_FILES app/parent_ui/PreviewHost.qml)
    # A console program, so that the check's findings reach the test log.
    set_target_properties(wjs_parent_preview PROPERTIES WIN32_EXECUTABLE OFF)
    target_include_directories(wjs_parent_preview PRIVATE "${PROJECT_SOURCE_DIR}/src/app/parent")
    target_link_libraries(wjs_parent_preview
        PRIVATE
            wjs_parentplugin
            wjs_i18n
            wjs::lib
            Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            wjs::warnings
            wjs::sanitizers
            wjs::coverage)
    wjs_deploy_sanitizer_runtime(wjs_parent_preview)
    wjs_platform_deploy_qt_runtime(wjs_parent_preview "${CMAKE_CURRENT_SOURCE_DIR}/app/parent_ui")
    # The third-party notices as :/notices/NOTICES.txt, which the About page's
    # licences view shows (cmake/Notices.cmake): the check below then reads what
    # a package's application reads, and fails if the resource is missing.
    wjs_add_notices_resource(wjs_parent_preview)
    # The QML compiler's generated C++ and MSVC's C4702, as for the application's
    # own QML target (src/app/CMakeLists.txt).
    get_target_property(_wjs_parent_preview_sources wjs_parent_preview SOURCES)
    foreach(_wjs_source IN LISTS _wjs_parent_preview_sources)
        if(_wjs_source MATCHES "/[.]rcc/qmlcache/")
            set_source_files_properties("${_wjs_source}" PROPERTIES
                COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
        endif()
    endforeach()

    add_test(NAME parent.screens
        COMMAND wjs_parent_preview --check --out "${CMAKE_CURRENT_BINARY_DIR}/parent-screens")
    set_tests_properties(parent.screens PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
        TIMEOUT 180)

    # The parent area in all eight languages, Hebrew and Yiddish included: the
    # screens load and draw translated, Hebrew and Yiddish mirror the bar (its way
    # back is on the right), the six left-to-right ones keep it on the left, and
    # QML warns of nothing. Each language's catalogue is applied before the
    # screen loads, so a missing or empty catalogue fails the run.
    add_test(NAME parent.languages
        COMMAND wjs_parent_preview --languages --out "${CMAKE_CURRENT_BINARY_DIR}/parent-languages")
    set_tests_properties(parent.languages PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
        TIMEOUT 240)

    # The Family Assistant's screens in a stub shell (app/scan_ui): a program to
    # look at them with, and, with --check, a test that walks scan.ready, the
    # typed board, the check screen and assist.playing without a display, checks
    # what they say, and fails on any warning from QML. The screens are the
    # application's own files and the models are its own classes; the stub shell
    # carries the kit's QML and tokens, because the real shell belongs to the
    # application executable and its 3D scene needs a display. Not part of the
    # application.
    qt_policy(SET QTP0001 NEW)
    # The preview aliases the kit and Solo Play QML under qml/kit/ and qml/solo/
    # as plain resources, loaded by URL and never as module types, so the extra
    # directories need no qmldir: this is the policy's previous behaviour.
    qt_policy(SET QTP0004 OLD)
    add_executable(wjs_scan_preview
        app/scan_ui/preview_main.cpp)
    set_target_properties(wjs_scan_preview PROPERTIES AUTOMOC ON WIN32_EXECUTABLE OFF)

    set(WJS_SCAN_PREVIEW_KIT_DIR "${PROJECT_SOURCE_DIR}/src/app/qml/kit")
    set(WJS_SCAN_PREVIEW_KIT_FILES
        AppButton.qml
        Feedback.qml
        Icon.qml
        Jewel.qml
        Theme.qml
        TileFace.qml)
    set(WJS_SCAN_PREVIEW_KIT_SOURCES "")
    foreach(_file IN LISTS WJS_SCAN_PREVIEW_KIT_FILES)
        set_source_files_properties("${WJS_SCAN_PREVIEW_KIT_DIR}/${_file}"
            PROPERTIES QT_RESOURCE_ALIAS "qml/kit/${_file}")
        list(APPEND WJS_SCAN_PREVIEW_KIT_SOURCES "${WJS_SCAN_PREVIEW_KIT_DIR}/${_file}")
    endforeach()
    # The history's pictures: the same BoardThumb Solo Play's history uses.
    set_source_files_properties("${PROJECT_SOURCE_DIR}/src/app/qml/solo/BoardThumb.qml"
        PROPERTIES QT_RESOURCE_ALIAS "qml/solo/BoardThumb.qml")
    list(APPEND WJS_SCAN_PREVIEW_KIT_SOURCES
        "${PROJECT_SOURCE_DIR}/src/app/qml/solo/BoardThumb.qml")
    set_source_files_properties(
        "${WJS_SCAN_PREVIEW_KIT_DIR}/Theme.qml"
        "${WJS_SCAN_PREVIEW_KIT_DIR}/Feedback.qml"
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
    set_source_files_properties(app/scan_ui/PreviewHost.qml
        PROPERTIES QT_RESOURCE_ALIAS PreviewHost.qml)

    qt_add_qml_module(wjs_scan_preview
        URI WordJewelsSolver
        VERSION 1.0
        QML_FILES
            ${WJS_SCAN_PREVIEW_KIT_SOURCES}
            app/scan_ui/PreviewHost.qml
        SOURCES
            "${PROJECT_SOURCE_DIR}/src/app/solo/board_layout.cpp"
            "${PROJECT_SOURCE_DIR}/src/app/solo/solo_layout.cpp"
            "${PROJECT_SOURCE_DIR}/src/app/system_colour_scheme.cpp"
            "${PROJECT_SOURCE_DIR}/src/app/system_motion.cpp")

    target_include_directories(wjs_scan_preview PRIVATE
        "${PROJECT_SOURCE_DIR}/src/app"
        "${PROJECT_SOURCE_DIR}/src/app/scan"
        "${PROJECT_SOURCE_DIR}/src/app/solo")
    target_link_libraries(wjs_scan_preview
        PRIVATE
            wjs_scan_uiplugin
            wjs_i18n
            wjs_trace
            wjs::lib
            wjs::platform
            Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
            wjs::warnings
            wjs::sanitizers
            wjs::coverage)
    # The QML compiler's generated C++ and MSVC's C4702, as for the application's
    # own QML target (src/app/CMakeLists.txt).
    get_target_property(_wjs_scan_preview_sources wjs_scan_preview SOURCES)
    foreach(_wjs_source IN LISTS _wjs_scan_preview_sources)
        if(_wjs_source MATCHES "/[.]rcc/qmlcache/")
            set_source_files_properties("${_wjs_source}" PROPERTIES
                COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
        endif()
    endforeach()
    wjs_deploy_sanitizer_runtime(wjs_scan_preview)
    wjs_platform_deploy_qt_runtime(wjs_scan_preview "${CMAKE_CURRENT_SOURCE_DIR}/app/scan_ui")

    add_test(NAME scan.screens
        COMMAND wjs_scan_preview --check --out "${CMAKE_CURRENT_BINARY_DIR}/scan-screens")
    set_tests_properties(scan.screens PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
        TIMEOUT 180)

    # Mode A in all eight languages, Hebrew and Yiddish included: the ready and
    # check screens and the assistant's panel are translated, the split puts the
    # panel on the trailing side (the left in RTL, the right in LTR) while the
    # replicated board stays as it was confirmed, and QML warns of nothing.
    add_test(NAME scan.languages
        COMMAND wjs_scan_preview --languages --out "${CMAKE_CURRENT_BINARY_DIR}/scan-languages")
    set_tests_properties(scan.languages PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
        TIMEOUT 240)

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
            -P "${CMAKE_CURRENT_SOURCE_DIR}/i18n/check_catalogues.cmake")
    set_tests_properties(i18n.catalogues PROPERTIES SKIP_RETURN_CODE 4)

    # The board scene is one component with one interface; the application's 3D
    # scene and the 2D fallback kept for D2 are each held to it
    # (docs/design/shake-prototype.md, "The interface"). QML has no interface type.
    add_test(NAME app.scene_contract
        COMMAND "${CMAKE_COMMAND}"
            "-DSCENES=${PROJECT_SOURCE_DIR}/src/app/qml/scene/BoardScene.qml;${PROJECT_SOURCE_DIR}/src/app/qml/scene/fallback/BoardScene.qml"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/app/check_scene_contract.cmake")

    # No warning from qmllint over the application's QML, or over the board
    # scene's module. It needs the types the build generates, so it skips itself
    # (exit 4) when the application is not built.
    if(TARGET Qt6::qmllint)
        add_test(NAME app.qml_lint
            COMMAND "${CMAKE_COMMAND}"
                "-DQMLLINT=$<TARGET_FILE:Qt6::qmllint>"
                "-DRESPONSE=${PROJECT_BINARY_DIR}/src/app/.rcc/qmllint/wjs_app.rsp"
                "-DTYPES=${PROJECT_BINARY_DIR}/src/app/WordJewelsSolver/wjs_app.qmltypes"
                "-DSCENE_RESPONSE=${PROJECT_BINARY_DIR}/src/app/scene/.rcc/qmllint/wjs_scene.rsp"
                "-DSCENE_TYPES=${PROJECT_BINARY_DIR}/src/app/scene/WordJewelsSolver/Scene/wjs_scene.qmltypes"
                "-DSCAN_RESPONSE=${PROJECT_BINARY_DIR}/src/app/scan/.rcc/qmllint/wjs_scan_ui.rsp"
                "-DSCAN_TYPES=${PROJECT_BINARY_DIR}/src/app/scan/WordJewelsSolver/Scan/wjs_scan_ui.qmltypes"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/app/run_qmllint.cmake")
        set_tests_properties(app.qml_lint PROPERTIES SKIP_RETURN_CODE 4)
    endif()
endif()

# The shake prototype's dice simulation: plain C++, tested without Qt or a window.
if(TARGET wjs_dice_world)
    add_executable(wjs_dice_tests prototype/dice_faces_test.cpp prototype/dice_world_test.cpp)
    target_link_libraries(wjs_dice_tests
        PRIVATE
            wjs_dice_world
            wjs::warnings
            wjs::sanitizers
            wjs::coverage
            Catch2::Catch2WithMain)
    wjs_deploy_sanitizer_runtime(wjs_dice_tests)
    catch_discover_tests(wjs_dice_tests)
endif()

# The third-party notices (cmake/Notices.cmake): every section a package must
# state, and no placeholder left unfilled. Runs on every host; a build with the
# application names Qt and Quick 3D, and a command-line package does not claim
# to carry either.
add_test(NAME notices.sections
    COMMAND "${CMAKE_COMMAND}"
        "-DNOTICES=${WJS_NOTICES_FILE}"
        "-DVERSION=${PROJECT_VERSION}"
        "-DEXPECT_QT=${WJS_BUILD_APP}"
        "-DQT_VERSION=${Qt6_VERSION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/notices/check_notices.cmake")

# The CPack configuration and the install rules it packages: both programs this
# build makes, the files they read, and the notices. It reads the generated
# CPackConfig.cmake and cmake_install.cmake scripts, so a dropped install() rule
# is a failed test rather than a package missing a file (docs/building.md,
# "Packaging"). Cheap, so it runs in every ctest.
add_test(NAME package.config
    COMMAND "${CMAKE_COMMAND}"
        "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
        "-DVERSION=${PROJECT_VERSION}"
        "-DEXPECT_CLI=${WJS_BUILD_CLI}"
        "-DEXPECT_APP=${WJS_BUILD_APP}"
        "-DEXPECT_QT_DEPLOY=${WJS_PLATFORM_QT_DEPLOY_AT_INSTALL}"
        "-DNOTICES=${WJS_NOTICES_FILE}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/package/check_package.cmake")

# Every platform fragment satisfies the platform contract, on whichever host
# this runs (a build only includes the one fragment for its own platform).
add_test(NAME platform.fragments
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/platform/check_fragments.cmake")

# Nothing that ships has Tracy in it: only the *-profiling presets turn the
# option on, it defaults to OFF, and the manifest feature that installs it is
# neither a base dependency nor a default feature (docs/profiling.md). Runs on
# every host and in every CI leg, since it only reads the presets.
add_test(NAME profiling.shipped_presets
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/profiling/check_presets.cmake")

# Every zone is named <layer>.<thing> and once: one name is one source location.
add_test(NAME profiling.zone_names
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/profiling/check_zone_names.cmake")

# The privacy rules of the first release that the source can say whether they
# hold (docs/design/application.md, "Privacy, built in and checked"): no audio
# captured, no network used, nothing exported, one file written and only by the
# store. Runs on every host and in every CI leg, since it only reads the tree.
add_test(NAME privacy.sources
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/privacy/check_sources.cmake")

# The scan held to a table of cases on throw-away trees, so that a pattern that
# stopped matching is a failure and not a pass.
add_test(NAME privacy.sources.selftest
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/privacy"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/privacy/test_check_sources.cmake")

# The sentences of the parent area are in one file and are the design's, with
# none of the words an assessment, a grade or a gap is made of
# (docs/design/dashboard.md, "Wording"), and the check held to cases of its own.
add_test(NAME parent.wording
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/parent/check_wording.cmake")
add_test(NAME parent.wording.selftest
    COMMAND "${CMAKE_COMMAND}"
        "-DREPO_ROOT=${PROJECT_SOURCE_DIR}"
        "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/parent-wording"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/parent/test_check_wording.cmake")

# No preprocessor conditionals or defines, no OS headers outside the platform
# section, no OS tests in CMake outside the selector. CI runs it as its own job
# too; this is the same check, so a developer sees it fail locally.
find_program(WJS_PWSH NAMES pwsh)
if(WJS_PWSH)
    # The word lists: lowercase a-z, sorted, unique, and every category and
    # tier word is in the dictionary.
    # The project's dice are what tools/data/make_dice.ps1 makes from the
    # dictionary as it is now: the source hash and the dice both have to match.
    add_test(NAME data.dice
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/data/make_dice.ps1"
            -Root "${PROJECT_SOURCE_DIR}" -Check)

    add_test(NAME data.wordlists
        COMMAND "${WJS_PWSH}" -NoProfile -File
            "${PROJECT_SOURCE_DIR}/tools/data/check_wordlists.ps1"
            -Root "${PROJECT_SOURCE_DIR}")

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
endif()
