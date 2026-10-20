# The application's model, built from its source so the test does not need the
# QML module (QML_ELEMENT registration is inert without it). Qt is only here
# when the application is.
add_executable(wjs_app_tests
    assist_game_test.cpp
    board_layout_test.cpp
    camera_test.cpp
    fake_solo_game_test.cpp
    feedback_engine_test.cpp
    language_test.cpp
    motion_source_test.cpp
    parent_controller_test.cpp
    parent_ui/demo_data.cpp
    path_tracer_test.cpp
    real_solo_game_test.cpp
    scan_model_test.cpp
    solo_model_test.cpp
    solver_model_test.cpp
    system_colour_scheme_test.cpp
    system_motion_test.cpp
    "${PROJECT_SOURCE_DIR}/src/app/camera/camera_source.cpp"
    "${PROJECT_SOURCE_DIR}/src/app/camera/frame_conversion.cpp"
    "${WJS_CAMERA_VARIANT_DIR}/wjs/app/camera_backend.cpp"
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
    "${PROJECT_SOURCE_DIR}/src/app/camera"
    "${PROJECT_SOURCE_DIR}/src/app/parent"
    "${PROJECT_SOURCE_DIR}/src/app/scan"
    "${PROJECT_SOURCE_DIR}/src/app/solo"
    "${CMAKE_CURRENT_SOURCE_DIR}/parent_ui"
    "${WJS_TESTS_DIR}/scan/synth"
    "${WJS_CAMERA_VARIANT_DIR}")
# The shipped data, by its path in this checkout, for the case that plays a
# standard board over the real dice and dictionary (real_solo_game_test.cpp),
# and the catalogues the interface's languages are read from
# (language_test.cpp).
target_compile_definitions(wjs_app_tests PRIVATE
    "WJS_DATA_DIRECTORY=\"${PROJECT_SOURCE_DIR}/data\""
    "WJS_I18N_DIRECTORY=\"${PROJECT_SOURCE_DIR}/i18n\"")
target_link_libraries(wjs_app_tests
    PRIVATE
        wjs_feedback
        wjs_scan_synth
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
# The multimedia backend's header is the only file that needs the module;
# the tests that hand in a fake backend do not. A kit without Qt Multimedia
# builds the none backend and this block is skipped.
if(WJS_QT_HAS_MULTIMEDIA)
    target_sources(wjs_app_tests PRIVATE camera_video_frame_test.cpp)
    target_link_libraries(wjs_app_tests PRIVATE Qt6::Multimedia)
endif()
wjs_deploy_sanitizer_runtime(wjs_app_tests)
# Qt's DLLs have to be beside the binary before catch_discover_tests runs it.
wjs_platform_deploy_qt_runtime(wjs_app_tests "")
# The tags are CTest labels too, so the camera's cases are `ctest -L camera`
# as well as part of every run.
# The window's model tier: the application's own classes, over the real
# library and with Qt. Named `integration.<sentence>`; the tier is selected
# by `ctest -R "^integration\\."` because Catch2's tags are this suite's
# CTest labels (`-L camera`, `-L app`, `-L parent`, ...).
catch_discover_tests(wjs_app_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS)

# The 3D board scene's script (src/app/qml/scene/ShakeScript.js) in Qt's own
# JavaScript engine, so what is tested is the file the scene plays and not a
# copy, with the dice simulation (DiceSim) it drives. The checks are in
# shake_script_check.js. They are about the dice's faces: that a die lands with
# the face the library's roll says is up, shows other faces on the way, and that
# none of it moves a die before it is sent home. Built with the application,
# whose scene the script is.
add_executable(wjs_shake_script_tests
    shake_script_test.cpp
    "${PROJECT_SOURCE_DIR}/src/app/scene/dice_sim.cpp")
set_target_properties(wjs_shake_script_tests PROPERTIES AUTOMOC ON)
target_include_directories(wjs_shake_script_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/src/app/scene"
    # budgets.hpp: the scene's work budgets are in the tier's one file.
    "${WJS_TESTS_DIR}/perf")
target_compile_definitions(wjs_shake_script_tests PRIVATE
    "WJS_SHAKE_SCRIPT=\"${PROJECT_SOURCE_DIR}/src/app/qml/scene/ShakeScript.js\""
    "WJS_SHAKE_SCRIPT_CHECK=\"${CMAKE_CURRENT_SOURCE_DIR}/shake_script_check.js\"")
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
catch_discover_tests(wjs_shake_script_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS)

# The window's frame budget (tests/app/frame_timing_test.cpp): the
# application's own smoke run in a child process, and the report it prints.
# Its own binary because it makes a QCoreApplication for QProcess and must
# not share one with the model tests. It skips itself unless a clock reading
# is about this build (an optimised, uninstrumented one) and somebody asked
# for it with WJS_PERF_FRAME_TIMING=1: a shared runner has no display and
# never sets it. RUN_SERIAL, because a frame budget is a stopwatch.
add_executable(wjs_frame_timing_tests frame_timing_test.cpp)
target_include_directories(wjs_frame_timing_tests PRIVATE "${WJS_TESTS_DIR}/perf")
target_compile_definitions(wjs_frame_timing_tests PRIVATE
    "WJS_APP_PATH=\"$<TARGET_FILE:WordJewelsSolver>\""
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
    TEST_PREFIX "system.frame."
    PROPERTIES RUN_SERIAL TRUE TIMEOUT 240 LABELS "system;perf;gui")

# The parent area's screens in a stub shell (app/parent_ui): a program to
# look at them with, and, with --check, a test that walks the gate, the
# dashboard, the settings and the about page without a display, checks what
# they say, and fails on any warning from QML. Not part of the application.
qt_policy(SET QTP0001 NEW)
qt_add_executable(wjs_parent_preview
    parent_ui/demo_data.cpp
    parent_ui/preview_main.cpp)
set_target_properties(wjs_parent_preview PROPERTIES AUTOMOC ON)
set_source_files_properties(parent_ui/PreviewHost.qml PROPERTIES QT_RESOURCE_ALIAS PreviewHost.qml)
qt_add_qml_module(wjs_parent_preview
    URI ParentPreview
    VERSION 1.0
    QML_FILES parent_ui/PreviewHost.qml)
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
wjs_platform_deploy_qt_runtime(wjs_parent_preview "${CMAKE_CURRENT_SOURCE_DIR}/parent_ui")
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
    TIMEOUT 180
    LABELS "system;ui;gui")

# The parent area in all eight languages, Hebrew and Yiddish included: the
# screens load and draw translated, Hebrew and Yiddish mirror the bar (its way
# back is on the right), the six left-to-right ones keep it on the left, and
# QML warns of nothing. Each language's catalogue is applied before the
# screen loads, so a missing or empty catalogue fails the run.
add_test(NAME parent.languages
    COMMAND wjs_parent_preview --languages --out "${CMAKE_CURRENT_BINARY_DIR}/parent-languages")
set_tests_properties(parent.languages PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
    TIMEOUT 240
    LABELS "system;ui;gui")

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
qt_policy(SET QTP0004 NEW)
add_executable(wjs_scan_preview
    scan_ui/preview_main.cpp)
set_target_properties(wjs_scan_preview PROPERTIES AUTOMOC ON WIN32_EXECUTABLE OFF)

set(WJS_SCAN_PREVIEW_KIT_DIR "${PROJECT_SOURCE_DIR}/src/app/qml/kit")
set(WJS_SCAN_PREVIEW_KIT_FILES
    AppBar.qml
    AppButton.qml
    Feedback.qml
    Icon.qml
    Jewel.qml
    Theme.qml
    TileFace.qml
    Wordmark.qml)
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
set_source_files_properties(scan_ui/PreviewHost.qml
    PROPERTIES QT_RESOURCE_ALIAS PreviewHost.qml)

qt_add_qml_module(wjs_scan_preview
    URI WordJewelsSolver
    VERSION 1.0
    QML_FILES
        ${WJS_SCAN_PREVIEW_KIT_SOURCES}
        scan_ui/PreviewHost.qml
    SOURCES
        "${PROJECT_SOURCE_DIR}/src/app/solo/board_layout.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/solo/solo_layout.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/system_colour_scheme.cpp"
        "${PROJECT_SOURCE_DIR}/src/app/system_motion.cpp")

target_include_directories(wjs_scan_preview PRIVATE
    "${PROJECT_SOURCE_DIR}/src/app"
    "${PROJECT_SOURCE_DIR}/src/app/camera"
    "${PROJECT_SOURCE_DIR}/src/app/scan"
    "${PROJECT_SOURCE_DIR}/src/app/solo"
    "${WJS_TESTS_DIR}/scan/synth"
    "${WJS_CAMERA_VARIANT_DIR}")
target_link_libraries(wjs_scan_preview
    PRIVATE
        wjs_scan_uiplugin
        wjs_scan_synth
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
# The live view imports QtMultimedia from the camera variant, which is not
# under the stub shell's qml directory.
set(WJS_SCAN_PREVIEW_DEPLOY_QML_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/scan_ui")
if(WJS_QT_HAS_MULTIMEDIA)
    list(APPEND WJS_SCAN_PREVIEW_DEPLOY_QML_DIRS "${WJS_CAMERA_VARIANT_DIR}")
endif()
wjs_platform_deploy_qt_runtime(wjs_scan_preview ${WJS_SCAN_PREVIEW_DEPLOY_QML_DIRS})

add_test(NAME scan.screens
    COMMAND wjs_scan_preview --check --out "${CMAKE_CURRENT_BINARY_DIR}/scan-screens")
set_tests_properties(scan.screens PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
    TIMEOUT 180
    LABELS "system;ui;gui")

# Mode A's camera without a camera: a fake source hands over one frame from
# the synthetic corpus, the live view is shown, and Capture opens the check
# screen. No device is opened, and only a kit with Qt Multimedia has the
# VideoOutput the preview is made of.
if(WJS_QT_HAS_MULTIMEDIA)
    add_test(NAME camera.screens
        COMMAND wjs_scan_preview --camera --out "${CMAKE_CURRENT_BINARY_DIR}/camera-screens")
    set_tests_properties(camera.screens PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
        TIMEOUT 180
        LABELS "system;ui;camera;gui")
endif()

# Mode A in all eight languages, Hebrew and Yiddish included: the ready and
# check screens and the assistant's panel are translated, the split puts the
# panel on the trailing side (the left in RTL, the right in LTR) while the
# replicated board stays as it was confirmed, and QML warns of nothing.
add_test(NAME scan.languages
    COMMAND wjs_scan_preview --languages --out "${CMAKE_CURRENT_BINARY_DIR}/scan-languages")
set_tests_properties(scan.languages PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
    TIMEOUT 240
    LABELS "system;ui;gui")

# The board scene is one component with one interface; the application's 3D
# scene and the 2D fallback kept for D2 are each held to it
# (docs/design/shake-prototype.md, "The interface"). QML has no interface type.
add_test(NAME app.scene_contract
    COMMAND "${CMAKE_COMMAND}"
        "-DSCENES=${PROJECT_SOURCE_DIR}/src/app/qml/scene/BoardScene.qml;${PROJECT_SOURCE_DIR}/src/app/qml/scene/fallback/BoardScene.qml"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_scene_contract.cmake")
set_tests_properties(app.scene_contract PROPERTIES LABELS "quality")

# No warning from qmllint over the application's QML, or over the board
# scene's module. It needs the types the build generates, so it skips itself
# (exit 4) when the application is not built.
if(TARGET Qt6::qmllint)
    add_test(NAME app.qml_lint
        COMMAND "${CMAKE_COMMAND}"
            "-DQMLLINT=$<TARGET_FILE:Qt6::qmllint>"
            "-DRESPONSE=${PROJECT_BINARY_DIR}/src/app/.rcc/qmllint/WordJewelsSolver.rsp"
            "-DTYPES=${PROJECT_BINARY_DIR}/src/app/WordJewelsSolver/WordJewelsSolver.qmltypes"
            "-DSCENE_RESPONSE=${PROJECT_BINARY_DIR}/src/app/scene/.rcc/qmllint/wjs_scene.rsp"
            "-DSCENE_TYPES=${PROJECT_BINARY_DIR}/src/app/scene/WordJewelsSolver/Scene/wjs_scene.qmltypes"
            "-DSCAN_RESPONSE=${PROJECT_BINARY_DIR}/src/app/scan/.rcc/qmllint/wjs_scan_ui.rsp"
            "-DSCAN_TYPES=${PROJECT_BINARY_DIR}/src/app/scan/WordJewelsSolver/Scan/wjs_scan_ui.qmltypes"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_qmllint.cmake")
    set_tests_properties(app.qml_lint PROPERTIES SKIP_RETURN_CODE 4 LABELS "quality")
endif()
