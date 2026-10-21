# ---------------------------------------------------------------------------
# The system tier: the programs themselves, not the classes behind them. The
# command-line solver's options, exit codes and output; the window's start-up
# contract and its headless smoke run; and the install rules run for real into
# a throw-away prefix, where the installed wjs-cli is then executed.
#
# These are CMake scripts rather than Catch2 binaries because they start
# programs: no test framework state, no display, and the exit code is the thing
# asserted. Each case is its own ctest entry (`system.cli.<case>`,
# `system.app.<case>`, `system.install`) under the `system` label.
# ---------------------------------------------------------------------------
set(WJS_SYSTEM_SCRIPT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(WJS_BUILD_CLI)
    foreach(_case IN ITEMS
            help
            version
            output-every-time-1
            output-every-time-2
            output-every-time-3
            output-every-time-4
            solve
            min-length
            rarity
            repeat
            qu
            unknown-option
            missing-option-value
            bad-min-length
            bad-repeat
            bad-scoring
            no-board
            board-error
            dictionary-error)
        add_test(NAME "system.cli.${_case}"
            COMMAND "${CMAKE_COMMAND}"
                "-DCLI=$<TARGET_FILE:wjs-cli>"
                "-DDICTIONARY=${PROJECT_SOURCE_DIR}/data/dictionaries/en.txt"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli"
                "-DCASE=${_case}"
                -P "${WJS_SYSTEM_SCRIPT_DIR}/check_cli.cmake")
        set_tests_properties("system.cli.${_case}" PROPERTIES
            LABELS "system"
            TIMEOUT 60)
    endforeach()
endif()

if(WJS_BUILD_APP)
    foreach(_case IN ITEMS
            missing-dictionary
            damaged-store
            phase-without-directory
            smoke-headless)
        add_test(NAME "system.app.${_case}"
            COMMAND "${CMAKE_COMMAND}"
                "-DAPP=$<TARGET_FILE:WordJewelsSolver>"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/app"
                "-DCASE=${_case}"
                -P "${WJS_SYSTEM_SCRIPT_DIR}/check_app.cmake")
        set_tests_properties("system.app.${_case}" PROPERTIES
            LABELS "system"
            TIMEOUT 150)
    endforeach()
endif()

# The install rules are always configured (the CLI may or may not be built), so
# this case is registered whenever the CLI is: it is the CLI's installed copy
# that proves the dictionary and notices landed beside it.
if(WJS_BUILD_CLI)
    add_test(NAME system.install
        COMMAND "${CMAKE_COMMAND}"
            "-DBUILD_DIR=${PROJECT_BINARY_DIR}"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/install"
            "-DCONFIG=$<CONFIG>"
            "-DEXE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}"
            "-DEXPECT_CLI=${WJS_BUILD_CLI}"
            "-DEXPECT_APP=${WJS_BUILD_APP}"
            "-DAPP_RELATIVE=${WJS_PLATFORM_INSTALL_APP_DATA_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
            "-DRUNTIME_DIR=$<TARGET_FILE_DIR:wjs-cli>"
            -P "${WJS_SYSTEM_SCRIPT_DIR}/check_install.cmake")
    set_tests_properties(system.install PROPERTIES
        LABELS "system"
        TIMEOUT 300)
endif()
