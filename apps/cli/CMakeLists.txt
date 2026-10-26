# The target is named after the program, as the application's is, so
# `--target sns-cli` is what a person types and the IDE shows the program names.
add_executable(sns-cli src/main.cpp)

target_link_libraries(sns-cli
    PRIVATE
        sns::lib
        sns::platform
        sns::warnings
        sns::hardening
        sns::sanitizers
        sns::coverage)

sns_deploy_sanitizer_runtime(sns-cli)

# The default dictionary the program looks for: dictionaries/en.txt beside the
# executable. A build copies it there; packaging will install it the same way.
add_custom_command(TARGET sns-cli POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:sns-cli>/dictionaries"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
        "$<TARGET_FILE_DIR:sns-cli>/dictionaries/en.txt"
    COMMENT "Placing the English dictionary beside sns-cli")

# The install rule, and the dictionary the way the program looks for it (the
# same relative path the application uses): a package is the build tree's bin/
# with Qt's runtime and the notices added, so what runs from build/<preset>/bin
# also runs from an installed copy (docs/building.md, "Packaging").
# CMAKE_INSTALL_BINDIR comes from GNUInstallDirs, which cmake/Platform.cmake
# includes.
install(TARGETS sns-cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/dictionaries")
