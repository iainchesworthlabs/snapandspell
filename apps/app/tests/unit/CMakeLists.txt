# The application's model, built from its source so the test does not need the
# QML module (QML_ELEMENT registration is inert without it). Qt is only here
# when the application is.
add_executable(sns_app_tests
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
    "${SNS_CAMERA_VARIANT_DIR}/sns/app/camera_backend.cpp"
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
set_target_properties(sns_app_tests PROPERTIES AUTOMOC ON)
target_include_directories(sns_app_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/apps/app/src"
    "${PROJECT_SOURCE_DIR}/apps/app/src/camera"
    "${PROJECT_SOURCE_DIR}/apps/app/src/parent"
    "${PROJECT_SOURCE_DIR}/apps/app/src/scan"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo"
    "${CMAKE_CURRENT_SOURCE_DIR}/parent_ui"
    "${PROJECT_SOURCE_DIR}/libs/scan/tests/synth"
    "${SNS_CAMERA_VARIANT_DIR}")
# The shipped data, by its path in this checkout, for the case that plays a
# standard board over the real dice and dictionary (real_solo_game_test.cpp),
# and the catalogues the interface's languages are read from
# (language_test.cpp).
target_compile_definitions(sns_app_tests PRIVATE
    "SNS_DATA_DIRECTORY=\"${PROJECT_SOURCE_DIR}/data\""
    "SNS_I18N_DIRECTORY=\"${PROJECT_SOURCE_DIR}/apps/app/translations\""
    "SNS_FONTS_DIRECTORY=\"${PROJECT_SOURCE_DIR}/assets/fonts\"")
target_link_libraries(sns_app_tests
    PRIVATE
        sns_feedback
        sns_scan_synth
        sns_trace
        sns_i18n
        sns::lib
        sns::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        sns::warnings
        sns::sanitizers
        sns::coverage
        Catch2::Catch2WithMain)
# The multimedia backend's header is the only file that needs the module;
# the tests that hand in a fake backend do not. A kit without Qt Multimedia
# builds the none backend and this block is skipped.
if(SNS_QT_HAS_MULTIMEDIA)
    target_sources(sns_app_tests PRIVATE
        camera_backend_test.cpp
        camera_video_frame_test.cpp)
    target_link_libraries(sns_app_tests PRIVATE Qt6::Multimedia)
endif()
sns_deploy_sanitizer_runtime(sns_app_tests)
# Qt's DLLs have to be beside the binary before catch_discover_tests runs it.
sns_platform_deploy_qt_runtime(sns_app_tests "")
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
list(LENGTH SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT _sns_headless_entries)
if(_sns_headless_entries GREATER 1)
    message(FATAL_ERROR
        "SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT has ${_sns_headless_entries} entries; the "
        "app tests' discovery takes one. Extend apps/app/tests/unit/CMakeLists.txt before adding a second.")
endif()
set(_sns_app_test_properties ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
if(_sns_headless_entries EQUAL 1)
    string(REGEX REPLACE "^([^=]+)=(.*)$" "\\1=set:\\2" _sns_headless_modification
        "${SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT}")
    list(APPEND _sns_app_test_properties ENVIRONMENT_MODIFICATION "${_sns_headless_modification}")
endif()
catch_discover_tests(sns_app_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS
    PROPERTIES ${_sns_app_test_properties})

# The board scene's script (apps/app/qml/solo/SceneScript.js) in Qt's own JavaScript engine, so
# what is tested is the file the scene plays and not a copy: the stepped hops of the box, the lid's
# quarters, the six frames of a die's fall, the seeded order and the length of a play. With it, the
# profiler seam (profiler_test.cpp), the QML object that opens and closes a Tracy zone. Built with the
# application, whose scene the script is.
add_executable(sns_scene_script_tests
    profiler_test.cpp
    scene_script_test.cpp
    "${PROJECT_SOURCE_DIR}/apps/app/src/scene/profiler.cpp")
set_target_properties(sns_scene_script_tests PROPERTIES AUTOMOC ON)
target_include_directories(sns_scene_script_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/apps/app/src/scene"
    # budgets.hpp: the scene's play budget is in the tier's one file.
    "${PROJECT_SOURCE_DIR}/libs/core/bench")
target_compile_definitions(sns_scene_script_tests PRIVATE
    "SNS_SCENE_SCRIPT=\"${PROJECT_SOURCE_DIR}/apps/app/qml/solo/SceneScript.js\"")
target_link_libraries(sns_scene_script_tests
    PRIVATE
        Qt6::Core
        Qt6::Qml
        sns::profiling
        sns::warnings
        sns::sanitizers
        sns::coverage
        Catch2::Catch2WithMain)
sns_deploy_sanitizer_runtime(sns_scene_script_tests)
sns_platform_deploy_qt_runtime(sns_scene_script_tests "")
# Every tag is a CTest label, so the scene's play budget (tagged perf) is part of `ctest -L perf` as
# the tier's own cases are.
catch_discover_tests(sns_scene_script_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS
    PROPERTIES COST 4)

# The window's frame budget (apps/app/tests/unit/frame_timing_test.cpp): the
# application's own smoke run in a child process, and the report it prints.
# Its own binary because it makes a QCoreApplication for QProcess and must
# not share one with the model tests. It skips itself unless a clock reading
# is about this build (an optimised, uninstrumented one) and somebody asked
# for it with SNS_PERF_FRAME_TIMING=1: a shared runner has no display and
# never sets it. RUN_SERIAL, because a frame budget is a stopwatch.
add_executable(sns_frame_timing_tests frame_timing_test.cpp)
target_include_directories(sns_frame_timing_tests PRIVATE "${PROJECT_SOURCE_DIR}/libs/core/bench")
target_compile_definitions(sns_frame_timing_tests PRIVATE
    "SNS_APP_PATH=\"$<TARGET_FILE:SnapAndSpell>\""
    "SNS_TEST_OPTIMISED=$<IF:$<CONFIG:Release,RelWithDebInfo,MinSizeRel>,1,0>"
    "SNS_TEST_INSTRUMENTED=$<IF:$<OR:$<BOOL:${SNS_ENABLE_COVERAGE}>,$<BOOL:${SNS_ENABLE_TRACY}>>,1,0>")
target_link_libraries(sns_frame_timing_tests
    PRIVATE
        Qt6::Core
        sns::warnings
        sns::sanitizers
        sns::coverage
        Catch2::Catch2WithMain)
sns_deploy_sanitizer_runtime(sns_frame_timing_tests)
sns_platform_deploy_qt_runtime(sns_frame_timing_tests "")
catch_discover_tests(sns_frame_timing_tests DISCOVERY_MODE PRE_TEST
    TEST_PREFIX "system.frame."
    PROPERTIES RUN_SERIAL TRUE TIMEOUT 240 LABELS "system;perf;gui")

# The window's frame clock (apps/app/src/frame_profiler.cpp) on a window that is
# never shown: the signals it listens to are emitted by the test, from a
# thread of its own for the render thread's. Its own binary because it needs
# Qt Quick and a GUI application on the offscreen platform.
add_executable(sns_frame_profiler_tests
    frame_profiler_test.cpp
    "${PROJECT_SOURCE_DIR}/apps/app/src/frame_profiler.cpp")
set_target_properties(sns_frame_profiler_tests PROPERTIES AUTOMOC ON)
target_include_directories(sns_frame_profiler_tests PRIVATE "${PROJECT_SOURCE_DIR}/apps/app/src")
target_link_libraries(sns_frame_profiler_tests
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        sns::profiling
        sns::warnings
        sns::sanitizers
        sns::coverage
        Catch2::Catch2WithMain)
sns_deploy_sanitizer_runtime(sns_frame_profiler_tests)
sns_platform_deploy_qt_runtime(sns_frame_profiler_tests "")
catch_discover_tests(sns_frame_profiler_tests
    TEST_PREFIX "integration."
    ADD_TAGS_AS_LABELS)

# The parent area's screens in a stub shell (app/parent_ui): a program to
# look at them with, and, with --check, a test that walks the gate, the
# dashboard, the settings and the about page without a display, checks what
# they say, and fails on any warning from QML. Not part of the application.
qt_policy(SET QTP0001 NEW)
# The parent screens use the kit's tokens and components (Theme, AppBar,
# AppButton, SegmentedChoice), so the shell carries the kit as the module
# SnapAndSpell, in a library of its own as apps/app/ui-tests does for its scene:
# the shell's own module is ParentPreview.
include("${PROJECT_SOURCE_DIR}/apps/app/qml/kit/kit_files.cmake")
set(SNS_PARENT_PREVIEW_KIT_SOURCES "")
foreach(_file IN LISTS SNS_KIT_QML_FILES)
    set_source_files_properties("${PROJECT_SOURCE_DIR}/apps/app/qml/kit/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND SNS_PARENT_PREVIEW_KIT_SOURCES "${PROJECT_SOURCE_DIR}/apps/app/qml/kit/${_file}")
endforeach()
foreach(_file IN LISTS SNS_KIT_QML_SINGLETONS)
    set_source_files_properties("${PROJECT_SOURCE_DIR}/apps/app/qml/kit/${_file}"
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
endforeach()
qt_add_library(sns_parent_preview_kit STATIC)
set_target_properties(sns_parent_preview_kit PROPERTIES AUTOMOC ON)
qt_add_qml_module(sns_parent_preview_kit
    URI SnapAndSpell
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/parent-preview-kit/SnapAndSpell"
    QML_FILES ${SNS_PARENT_PREVIEW_KIT_SOURCES}
    SOURCES
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_colour_scheme.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_colour_scheme.hpp")

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(sns_parent_preview_kit)
target_include_directories(sns_parent_preview_kit PUBLIC "${PROJECT_SOURCE_DIR}/apps/app/src")
target_link_libraries(sns_parent_preview_kit
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    PRIVATE
        sns::warnings
        sns::sanitizers
        sns::coverage)
get_target_property(_sns_parent_kit_sources sns_parent_preview_kit SOURCES)
foreach(_sns_source IN LISTS _sns_parent_kit_sources)
    if(_sns_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_sns_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()
qt_add_executable(sns_parent_preview
    parent_ui/demo_data.cpp
    parent_ui/preview_main.cpp)
set_target_properties(sns_parent_preview PROPERTIES AUTOMOC ON)
set_source_files_properties(parent_ui/PreviewHost.qml PROPERTIES QT_RESOURCE_ALIAS PreviewHost.qml)
qt_add_qml_module(sns_parent_preview
    URI ParentPreview
    VERSION 1.0
    QML_FILES parent_ui/PreviewHost.qml)

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(sns_parent_preview)
# A console program, so that the check's findings reach the test log.
set_target_properties(sns_parent_preview PROPERTIES WIN32_EXECUTABLE OFF)
target_include_directories(sns_parent_preview PRIVATE "${PROJECT_SOURCE_DIR}/apps/app/src/parent")
target_link_libraries(sns_parent_preview
    PRIVATE
        sns_parentplugin
        sns_parent_preview_kit
        sns_parent_preview_kitplugin
        sns_test_fonts
        sns_i18n
        sns::lib
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        sns::warnings
        sns::sanitizers
        sns::coverage)
sns_deploy_sanitizer_runtime(sns_parent_preview)
sns_platform_deploy_qt_runtime(sns_parent_preview "${CMAKE_CURRENT_SOURCE_DIR}/parent_ui")
# The third-party notices as :/notices/NOTICES.txt, which the About page's
# licences view shows (cmake/Notices.cmake): the check below then reads what
# a package's application reads, and fails if the resource is missing.
sns_add_notices_resource(sns_parent_preview)
# The QML compiler's generated C++ and MSVC's C4702, as for the application's
# own QML target (apps/app/CMakeLists.txt).
get_target_property(_sns_parent_preview_sources sns_parent_preview SOURCES)
foreach(_sns_source IN LISTS _sns_parent_preview_sources)
    if(_sns_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_sns_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()

add_test(NAME parent.screens
    COMMAND sns_parent_preview --check --out "${CMAKE_CURRENT_BINARY_DIR}/parent-screens")
set_tests_properties(parent.screens PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
    TIMEOUT 180
    LABELS "system;ui;gui")

# The parent area in all eight languages, Hebrew and Yiddish included: the
# screens load and draw translated, Hebrew and Yiddish mirror the bar (its way
# back is on the right), the six left-to-right ones keep it on the left, and
# QML warns of nothing. Each language's catalogue is applied before the
# screen loads, so a missing or empty catalogue fails the run.
add_test(NAME parent.languages
    COMMAND sns_parent_preview --languages --out "${CMAKE_CURRENT_BINARY_DIR}/parent-languages")
set_tests_properties(parent.languages PROPERTIES
    COST 30
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
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
add_executable(sns_scan_preview
    scan_ui/preview_main.cpp)
set_target_properties(sns_scan_preview PROPERTIES AUTOMOC ON WIN32_EXECUTABLE OFF)

# The whole kit, from the one list (apps/app/qml/kit/kit_files.cmake): a kit
# component that uses another fails to load in a preview that lacks it.
include("${PROJECT_SOURCE_DIR}/apps/app/qml/kit/kit_files.cmake")
set(SNS_SCAN_PREVIEW_KIT_DIR "${PROJECT_SOURCE_DIR}/apps/app/qml/kit")
set(SNS_SCAN_PREVIEW_KIT_FILES ${SNS_KIT_QML_FILES})
set(SNS_SCAN_PREVIEW_KIT_SOURCES "")
foreach(_file IN LISTS SNS_SCAN_PREVIEW_KIT_FILES)
    set_source_files_properties("${SNS_SCAN_PREVIEW_KIT_DIR}/${_file}"
        PROPERTIES QT_RESOURCE_ALIAS "${_file}")
    list(APPEND SNS_SCAN_PREVIEW_KIT_SOURCES "${SNS_SCAN_PREVIEW_KIT_DIR}/${_file}")
endforeach()
# The history's pictures: the same BoardThumb Solo Play's history uses.
set_source_files_properties("${PROJECT_SOURCE_DIR}/apps/app/qml/solo/BoardThumb.qml"
    PROPERTIES QT_RESOURCE_ALIAS "qml/solo/BoardThumb.qml")
list(APPEND SNS_SCAN_PREVIEW_KIT_SOURCES
    "${PROJECT_SOURCE_DIR}/apps/app/qml/solo/BoardThumb.qml")
foreach(_file IN LISTS SNS_KIT_QML_SINGLETONS)
    set_source_files_properties("${SNS_SCAN_PREVIEW_KIT_DIR}/${_file}"
        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
endforeach()
set_source_files_properties(scan_ui/PreviewHost.qml
    PROPERTIES QT_RESOURCE_ALIAS PreviewHost.qml)

qt_add_qml_module(sns_scan_preview
    URI SnapAndSpell
    VERSION 1.0
    QML_FILES
        ${SNS_SCAN_PREVIEW_KIT_SOURCES}
        scan_ui/PreviewHost.qml
    SOURCES
        "${PROJECT_SOURCE_DIR}/apps/app/src/solo/board_layout.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/solo/solo_layout.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_colour_scheme.cpp"
        "${PROJECT_SOURCE_DIR}/apps/app/src/system_motion.cpp")

# GCC rejects qmlcachegen's output under -Werror, so generated files are built without it.
icl_relax_generated_warnings(sns_scan_preview)

target_include_directories(sns_scan_preview PRIVATE
    "${PROJECT_SOURCE_DIR}/apps/app/src"
    "${PROJECT_SOURCE_DIR}/apps/app/src/camera"
    "${PROJECT_SOURCE_DIR}/apps/app/src/scan"
    "${PROJECT_SOURCE_DIR}/apps/app/src/solo"
    "${PROJECT_SOURCE_DIR}/libs/scan/tests/synth"
    "${SNS_CAMERA_VARIANT_DIR}")
target_link_libraries(sns_scan_preview
    PRIVATE
        sns_scan_uiplugin
        sns_scan_synth
        sns_media_probe
        sns_test_fonts
        sns_i18n
        sns_trace
        sns::lib
        sns::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        sns::warnings
        sns::sanitizers
        sns::coverage)
# The QML compiler's generated C++ and MSVC's C4702, as for the application's
# own QML target (apps/app/CMakeLists.txt).
get_target_property(_sns_scan_preview_sources sns_scan_preview SOURCES)
foreach(_sns_source IN LISTS _sns_scan_preview_sources)
    if(_sns_source MATCHES "/[.]rcc/qmlcache/")
        set_source_files_properties("${_sns_source}" PROPERTIES
            COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
    endif()
endforeach()
sns_deploy_sanitizer_runtime(sns_scan_preview)
# The live view imports QtMultimedia from the camera variant, which is not
# under the stub shell's qml directory.
set(SNS_SCAN_PREVIEW_DEPLOY_QML_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/scan_ui")
if(SNS_QT_HAS_MULTIMEDIA)
    list(APPEND SNS_SCAN_PREVIEW_DEPLOY_QML_DIRS "${SNS_CAMERA_VARIANT_DIR}")
endif()
sns_platform_deploy_qt_runtime(sns_scan_preview ${SNS_SCAN_PREVIEW_DEPLOY_QML_DIRS})

add_test(NAME scan.screens
    COMMAND sns_scan_preview --check --out "${CMAKE_CURRENT_BINARY_DIR}/scan-screens")
set_tests_properties(scan.screens PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
    TIMEOUT 180
    LABELS "system;ui;gui")

# Mode A's camera without a camera: a fake source hands over one frame from
# the synthetic corpus, the live view is shown, and Capture opens the check
# screen. No device is opened, and only a kit with Qt Multimedia has the
# VideoOutput the preview is made of.
if(SNS_QT_HAS_MULTIMEDIA)
    add_test(NAME camera.screens
        COMMAND sns_scan_preview --camera --out "${CMAKE_CURRENT_BINARY_DIR}/camera-screens")
    # Exit 77 is "Qt Multimedia has no backend on this machine" (tests/media/media_probe.hpp): a
    # skip, not a failure.
    set_tests_properties(camera.screens PROPERTIES
        SKIP_RETURN_CODE 77
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
        TIMEOUT 180
        LABELS "system;ui;camera;gui")
endif()

# Mode A in all eight languages, Hebrew and Yiddish included: the ready and
# check screens and the assistant's panel are translated, the split puts the
# panel on the trailing side (the left in RTL, the right in LTR) while the
# replicated board stays as it was confirmed, and QML warns of nothing.
add_test(NAME scan.languages
    COMMAND sns_scan_preview --languages --out "${CMAKE_CURRENT_BINARY_DIR}/scan-languages")
set_tests_properties(scan.languages PROPERTIES
    COST 30
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT}"
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
            "-DRESPONSE=${PROJECT_BINARY_DIR}/apps/app/.rcc/qmllint/SnapAndSpell.rsp"
            "-DTYPES=${PROJECT_BINARY_DIR}/apps/app/SnapAndSpell/SnapAndSpell.qmltypes"
            "-DSCAN_RESPONSE=${PROJECT_BINARY_DIR}/apps/app/src/scan/.rcc/qmllint/sns_scan_ui.rsp"
            "-DSCAN_TYPES=${PROJECT_BINARY_DIR}/apps/app/src/scan/SnapAndSpell/Scan/sns_scan_ui.qmltypes"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_qmllint.cmake")
    set_tests_properties(app.qml_lint PROPERTIES SKIP_RETURN_CODE 4 LABELS "quality")
endif()
