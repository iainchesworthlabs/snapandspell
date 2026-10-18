add_executable(wjs_cli main.cpp)
set_target_properties(wjs_cli PROPERTIES OUTPUT_NAME wjs-cli)

target_link_libraries(wjs_cli
    PRIVATE
        wjs::lib
        wjs::platform
        wjs::warnings
        wjs::sanitizers
        wjs::coverage)

wjs_deploy_sanitizer_runtime(wjs_cli)

# The default dictionary the program looks for: dictionaries/en.txt beside the
# executable. A build copies it there; packaging will install it the same way.
add_custom_command(TARGET wjs_cli POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:wjs_cli>/dictionaries"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
        "$<TARGET_FILE_DIR:wjs_cli>/dictionaries/en.txt"
    COMMENT "Placing the English dictionary beside wjs-cli")

# The install rule, and the dictionary the way the program looks for it (the
# same relative path the application uses): a package is the build tree's bin/
# with Qt's runtime and the notices added, so what runs from build/<preset>/bin
# also runs from an installed copy (docs/building.md, "Packaging").
# CMAKE_INSTALL_BINDIR comes from GNUInstallDirs, which cmake/Platform.cmake
# includes.
install(TARGETS wjs_cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/dictionaries")
