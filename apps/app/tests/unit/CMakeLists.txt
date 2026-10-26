# The application's model, built from its source so the test does not need the
# QML module (QML_ELEMENT registration is inert without it). Qt is only here
# when the application is.
add_executable(wjs_app_tests
    assist_game_test.cpp
    assist_model_test.cpp
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
    sound_sequence_player_test.cpp
    qt_text_test.cpp
    startup_failure_test.cpp
    system_colour_scheme_test.cpp
    system_motion_test.cpp
    "${PROJECT_SOURCE_DIR}/apps/app/src/camera/camera_source.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/camera/frame_conversion.cpp"
    "${WJS_CAMERA_VARIANT_DIR}/wjs/app/camera_backend.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/parent/parent_controller.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/scan/assist_game.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/scan/assist_game_factory.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/scan/assist_model.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/scan/scan_model.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/board_layout.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/fake_solo_game.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/real_solo_game.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_game.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_game_factory.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_game_setup.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_layout.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_model.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/startup_failure.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/system_colour_scheme.cpp"
    "${PROJECT_SOURCE_DIR}/apps/app/src/system_motion.cpp")
set_target_properties(wjs_app_tests PROPERTIES AUTOMOC ON)
target_include_directories(wjs_app_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/apps/app/src"
    "${PROJECT_SOURCE_DIR}/apps/app/src/camera"
    "${PROJECT_SOURCE_DIR}/apps/app/src/parent"
    "${PROJECT_SOURCE_DIR}/apps/app/src/scan"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo"
    "${CMAKE_CURRENT_SOURCE_DIR}/parent_ui"
    "${PROJECT_SOURCE_DIR}/libs/scan/tests/synth"
    "${WJS_CAMERA_VARIANT_DIR}")
# The shipped data, by its path in this checkout, for the case that plays a
# standard board over the real dice and dictionary (real_solo_game_test.cpp),
# and the catalogues the interface's languages are read from
# (language_test.cpp).
target_compile_definitions(wjs_app_tests PRIVATE
    "WJS_DATA_DIRECTORY=\"${PROJECT_SOURCE_DIR}/data\""
    "WJS_I18N_DIRECTORY=\"${PROJECT_SOURCE_DIR}/apps/app/translations\"")
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
    target_sources(wjs_app_tests PRIVATE
        camera_backend_test.cpp
        camera_video_frame_test.cpp)
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
#
# A case that makes a GUI application draws on the offscreen platform, chosen
# here, in the entry's environment, and not from inside the process: a case
# run by hand needs the same (docs/testing.md). The fonts directory is the one
# the platform's fragment names, for the cases that ask the font database. The
# discovery passes a property's value on as one list element, so the platform's
# NAME=value entry goes in the second property, the environment modification
# (NAME=set:value), and not in a ';'-separated list of the first.
list(LENGTH WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT _wjs_headless_entries)
if(_wjs_headless_entries GREATER 1)
    message(FATAL_ERROR
        "WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT has ${_wjs_headless_entries} entries; the "
        "app tests' discovery takes one. Extend apps/app/tests/unit/CMakeLists.txt before adding a second.")
endif()
set(_wjs_app_test_properties ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
if(_wjs_headless_entries EQUAL 1)
    string(REGEX REPLACE "^([^=]+)=(.*)$" "\\1=set:\\2" _wjs_headless_modification
        "${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}")
    list(APPEND _wjs_app_test_properties ENVIRONMENT_MODIFICATION "${_wjs_headless_modification}")
endif()
catch_discover_tests(wjs_app_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS
    PROPERTIES ${_wjs_app_test_properties})

# The board scene's script (apps/app/qml/solo/SceneScript.js) in Qt's own JavaScript engine, so
# what is tested is the file the scene plays and not a copy: the stepped hops of the box, the lid's
# quarters, the six frames of a die's fall, the seeded order and the length of a play. With it, the
# profiler seam (profiler_test.cpp), the QML object that opens and closes a Tracy zone. Built with the
# application, whose scene the script is.
add_executable(wjs_scene_script_tests
    profiler_test.cpp
    scene_script_test.cpp
    "${PROJECT_SOURCE_DIR}/apps/app/src/scene/profiler.cpp")
set_target_properties(wjs_scene_script_tests PROPERTIES AUTOMOC ON)
target_include_directories(wjs_scene_script_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/apps/app/src/scene"
    # budgets.hpp: the scene's play budget is in the tier's one file.
    "${PROJECT_SOURCE_DIR}/libs/core/bench")
target_compile_definitions(wjs_scene_script_tests PRIVATE
    "WJS_SCENE_SCRIPT=\"${PROJECT_SOURCE_DIR}/apps/app/qml/solo/SceneScript.js\"")
target_link_libraries(wjs_scene_script_tests
    PRIVATE
        Qt6::Core
        Qt6::Qml
        wjs::profiling
        wjs::warnings
        wjs::sanitizers
        wjs::coverage
        Catch2::Catch2WithMain)
wjs_deploy_sanitizer_runtime(wjs_scene_script_tests)
wjs_platform_deploy_qt_runtime(wjs_scene_script_tests "")
# Every tag is a CTest label, so the scene's play budget (tagged perf) is part of `ctest -L perf` as
# the tier's own cases are.
catch_discover_tests(wjs_scene_script_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS
    PROPERTIES COST 4)

# The window's frame budget (apps/app/tests/unit/frame_timing_test.cpp): the
# application's own smoke run in a child process, and the report it prints.
# Its own binary because it makes a QCoreApplication for QProcess and must
# not share one with the model tests. It skips itself unless a clock reading
# is about this build (an optimised, uninstrumented one) and somebody asked
# for it with WJS_PERF_FRAME_TIMING=1: a shared runner has no display and
# never sets it. RUN_SERIAL, because a frame budget is a stopwatch.
add_executable(wjs_frame_timing_tests frame_timing_test.cpp)
target_include_directories(wjs_frame_timing_tests PRIVATE "${PROJECT_SOURCE_DIR}/libs/core/bench")
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

# The window's frame clock (apps/app/src/frame_profiler.cpp) on a window that is
# never shown: the signals it listens to are emitted by the test, from a
# thread of its own for the render thread's. Its own binary because it needs
# Qt Quick and a GUI application on the offscreen platform.
add_executable(wjs_frame_profiler_tests
    frame_profiler_test.cpp
    "${PROJECT_SOURCE_DIR}/apps/app/src/frame_profiler.cpp")
set_target_properties(wjs_frame_profiler_tests PROPERTIES AUTOMOC ON)
target_include_directories(wjs_frame_profiler_tests PRIVATE "${PROJECT_SOURCE_DIR}/apps/app/src")
target_link_libraries(wjs_frame_profiler_tests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        wjs::profiling
        wjs::warnings
        wjs::sanitizers
        wjs::coverage
        Catch2::Catch2WithMain)
wjs_deploy_sanitizer_runtime(wjs_frame_profiler_tests)
wjs_platform_deploy_qt_runtime(wjs_frame_profiler_tests "")
catch_discover_tests(wjs_frame_profiler_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS)

# The parent area's screens in a stub shell (app/parent_ui): a program to
# look at them with, and, with --check, a test that walks the gate, the
# dashboard, the settings and the about page without a display, checks what
# they say, and fails on any warning from QML. Not part of the application.
qt_policy(SET QTP0001 NEW)
# The parent screens use the kit's tokens and components (Theme, AppBar,
# AppButton, SegmentedChoice), so the shell carries the kit as the module
# WordJewelsSolver, in a library of its own as apps/app/ui-tests does for its scene:
# the shell's own module is ParentPreview.
include("${PROJECT_SOURCE_DIR}/apps/app/qml/kit/kit_files.cmake")
set(WJS_PARENT_PREVIEW_KIT_SOURCES "")
foreach(_file IN LISTS WJS_KIT_QML_FILES)
    set_source_files_properties("${PROJECT_SOURCE_DIR}/apps/app/qml/kit/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND WJS_PARENT_PREVIEW_KIT_SOURCES "${PROJECT_SOURCE_DIR}/apps/app/qml/kit/${_file}")
endforeach()
foreach(_file IN LISTS WJS_KIT_QML_SINGLETONS)
    set_source_files_properties("${PROJECT_SOURCE_DIR}/apps/app/qml/kit/${_file}"
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
endforeach()
qt_add_library(wjs_parent_preview_kit STATIC)
set_target_properties(wjs_parent_preview_kit PROPERTIES AUTOMOC ON)
qt_add_qml_module(wjs_parent_preview_kit
    URI WordJewelsSolver
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/parent-preview-kit/WordJewelsSolver"
    QML_FILES ${WJS_PARENT_PREVIEW_KIT_SOURCES}
    SOURCES
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_colour_scheme.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_colour_scheme.hpp")

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(wjs_parent_preview_kit)
target_include_directories(wjs_parent_preview_kit PUBLIC "${PROJECT_SOURCE_DIR}/apps/app/src")
target_link_libraries(wjs_parent_preview_kit
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)
get_target_property(_wjs_parent_kit_sources wjs_parent_preview_kit SOURCES)
foreach(_wjs_source IN LISTS _wjs_parent_kit_sources)
    if(_wjs_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_wjs_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()
qt_add_executable(wjs_parent_preview
    parent_ui/demo_data.cpp
    parent_ui/preview_main.cpp)
set_target_properties(wjs_parent_preview PROPERTIES AUTOMOC ON)
set_source_files_properties(parent_ui/PreviewHost.qml PROPERTIES QT_RESOURCE_ALIAS PreviewHost.qml)
qt_add_qml_module(wjs_parent_preview
    URI ParentPreview
    VERSION 1.0
    QML_FILES parent_ui/PreviewHost.qml)

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(wjs_parent_preview)
# A console program, so that the check's findings reach the test log.
set_target_properties(wjs_parent_preview PROPERTIES WIN32_EXECUTABLE OFF)
target_include_directories(wjs_parent_preview PRIVATE "${PROJECT_SOURCE_DIR}/apps/app/src/parent")
target_link_libraries(wjs_parent_preview
    PRIVATE
        wjs_parentplugin
        wjs_parent_preview_kit
        wjs_parent_preview_kitplugin
        wjs_test_fonts
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
# own QML target (apps/app/CMakeLists.txt).
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
    COST 30
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
# The preview aliases the kit flat (as the parent preview does, so the one
# source property serves both) and Solo Play's BoardThumb under qml/solo/; the
# policy (a qmldir for every such directory) is NEW, its previous behaviour being
# deprecated.
qt_policy(SET QTP0004 NEW)
add_executable(wjs_scan_preview
    scan_ui/preview_main.cpp)
set_target_properties(wjs_scan_preview PROPERTIES AUTOMOC ON WIN32_EXECUTABLE OFF)

# The whole kit, from the one list (apps/app/qml/kit/kit_files.cmake): a kit
# component that uses another fails to load in a preview that lacks it.
include("${PROJECT_SOURCE_DIR}/apps/app/qml/kit/kit_files.cmake")
set(WJS_SCAN_PREVIEW_KIT_DIR "${PROJECT_SOURCE_DIR}/apps/app/qml/kit")
set(WJS_SCAN_PREVIEW_KIT_FILES ${WJS_KIT_QML_FILES})
set(WJS_SCAN_PREVIEW_KIT_SOURCES "")
foreach(_file IN LISTS WJS_SCAN_PREVIEW_KIT_FILES)
    set_source_files_properties("${WJS_SCAN_PREVIEW_KIT_DIR}/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND WJS_SCAN_PREVIEW_KIT_SOURCES "${WJS_SCAN_PREVIEW_KIT_DIR}/${_file}")
endforeach()
# The history's pictures: the same BoardThumb Solo Play's history uses.
set_source_files_properties("${PROJECT_SOURCE_DIR}/apps/app/qml/solo/BoardThumb.qml"
    PROPERTIES QT_RESOURCE_ALIAS "qml/solo/BoardThumb.qml")
list(APPEND WJS_SCAN_PREVIEW_KIT_SOURCES
    "${PROJECT_SOURCE_DIR}/apps/app/qml/solo/BoardThumb.qml")
foreach(_file IN LISTS WJS_KIT_QML_SINGLETONS)
    set_source_files_properties("${WJS_SCAN_PREVIEW_KIT_DIR}/${_file}"
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
endforeach()
set_source_files_properties(scan_ui/PreviewHost.qml
    PROPERTIES QT_RESOURCE_ALIAS PreviewHost.qml)

qt_add_qml_module(wjs_scan_preview
    URI WordJewelsSolver
    VERSION 1.0
    QML_FILES
        ${WJS_SCAN_PREVIEW_KIT_SOURCES}
        scan_ui/PreviewHost.qml
    SOURCES
        "${PROJECT_SOURCE_DIR}/apps/app/src/solo/board_layout.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_layout.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_colour_scheme.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_motion.cpp")

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(wjs_scan_preview)

target_include_directories(wjs_scan_preview PRIVATE
    "${PROJECT_SOURCE_DIR}/apps/app/src"
    "${PROJECT_SOURCE_DIR}/apps/app/src/camera"
    "${PROJECT_SOURCE_DIR}/apps/app/src/scan"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo"
    "${PROJECT_SOURCE_DIR}/libs/scan/tests/synth"
    "${WJS_CAMERA_VARIANT_DIR}")
target_link_libraries(wjs_scan_preview
    PRIVATE
        wjs_scan_uiplugin
        wjs_scan_synth
        wjs_media_probe
        wjs_test_fonts
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
# own QML target (apps/app/CMakeLists.txt).
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
    # Exit 77 is "Qt Multimedia has no backend on this machine" (tests/media/media_probe.hpp): a
    # skip, not a failure.
    set_tests_properties(camera.screens PROPERTIES
        SKIP_RETURN_CODE 77
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
    COST 30
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${WJS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
    TIMEOUT 240
    LABELS "system;ui;gui")

# The board scene is one component with one interface; the application's scene and the stub
# that the Quick Test suites give the Solo Play screen are each held to it
# (docs/design/board.md, "The shake and the drop"). QML has no interface type.
add_test(NAME app.scene_contract
    COMMAND "${CMAKE_COMMAND}"
        "-DSCENES=${PROJECT_SOURCE_DIR}/apps/app/qml/solo/BoardScene.qml;${PROJECT_SOURCE_DIR}/apps/app/ui-tests/StubBoardScene.qml"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_scene_contract.cmake")
set_tests_properties(app.scene_contract PROPERTIES LABELS "quality")

# No warning from qmllint over the application's QML (the board scene is part of it). It needs the
# types the build generates, so it skips itself (exit 4) when the application is not built.
if(TARGET Qt6::qmllint)
    add_test(NAME app.qml_lint
        COMMAND "${CMAKE_COMMAND}"
            "-DQMLLINT=$<TARGET_FILE:Qt6::qmllint>"
            "-DRESPONSE=${PROJECT_BINARY_DIR}/apps/app/.rcc/qmllint/WordJewelsSolver.rsp"
            "-DTYPES=${PROJECT_BINARY_DIR}/apps/app/WordJewelsSolver/WordJewelsSolver.qmltypes"
            "-DSCAN_RESPONSE=${PROJECT_BINARY_DIR}/apps/app/src/scan/.rcc/qmllint/wjs_scan_ui.rsp"
            "-DSCAN_TYPES=${PROJECT_BINARY_DIR}/apps/app/src/scan/WordJewelsSolver/Scan/wjs_scan_ui.qmltypes"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_qmllint.cmake")
    set_tests_properties(app.qml_lint PROPERTIES SKIP_RETURN_CODE 4 LABELS "quality")
endif()
