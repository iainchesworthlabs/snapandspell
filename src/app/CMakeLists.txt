# Qt6 is found by the top-level CMakeLists.txt.

# AUTOMOC and friends, and the QML module plumbing.
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(wjs_app
    main.cpp)
set_target_properties(wjs_app PROPERTIES OUTPUT_NAME WordJewelsSolver)

qt_add_qml_module(wjs_app
    URI WordJewelsSolver
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    SOURCES
        solver_model.cpp
        solver_model.hpp)

target_link_libraries(wjs_app
    PRIVATE
        wjs::lib
        wjs::platform
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

# What makes it a program of the platform's desktop, and its Qt runtime beside
# it: both are the selected platform's to say (src/platforms/<name>/).
wjs_platform_configure_app(wjs_app)
wjs_platform_deploy_qt_runtime(wjs_app "${CMAKE_CURRENT_SOURCE_DIR}/qml")
wjs_deploy_sanitizer_runtime(wjs_app)
