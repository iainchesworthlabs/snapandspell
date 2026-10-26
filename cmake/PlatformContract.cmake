# SPDX-License-Identifier: GPL-3.0-or-later
# ---------------------------------------------------------------------------
# PlatformContract.cmake - what a platform fragment
# (libs/platforms/<name>/platform.cmake) has to declare, and the check that it
# did. The contract itself is described at the top of cmake/Platform.cmake.
#
# Shared by the selector (which checks the one fragment it includes, at
# configure time, so a platform that forgets part of the contract fails by
# name) and by tests/platform/check_fragments.cmake (which checks every
# fragment in the tree on every host, so the platforms a leg does not build
# are still held to it).
# ---------------------------------------------------------------------------
include_guard(GLOBAL)

set(SNS_PLATFORM_CONTRACT_VARIABLES
    SNS_PLATFORM_DISPLAY_NAME
    SNS_PLATFORM_DESKTOP
    SNS_PLATFORM_EXECUTABLE_FORMAT
    SNS_PLATFORM_QT_SEARCH_ROOTS
    SNS_PLATFORM_QT_ARCH_DIRS
    SNS_PLATFORM_QT_HEADLESS_ENVIRONMENT
    SNS_PLATFORM_INSTALL_APP_DATA_DIR
    SNS_PLATFORM_QT_DEPLOY_AT_INSTALL
    SNS_PLATFORM_SOURCES
    SNS_PLATFORM_LINK_LIBRARIES
    SNS_PLATFORM_COMPILE_DEFINITIONS)

set(SNS_PLATFORM_CONTRACT_HOOKS
    sns_platform_configure_app
    sns_platform_deploy_qt_runtime
    sns_platform_configure_packaging)

# Checks the fragment that was just included (in the caller's scope):
#   - every contract variable is defined (an empty string is a definition);
#   - every hook exists;
#   - SNS_PLATFORM_DESKTOP is ON or OFF;
#   - SNS_PLATFORM_EXECUTABLE_FORMAT is elf, pe, mach-o or wasm;
#   - every file the fragment compiles exists under <platforms_dir>.
# `fragment` only names it in the message.
function(sns_platform_check_contract fragment platforms_dir)
    foreach(_variable IN LISTS SNS_PLATFORM_CONTRACT_VARIABLES)
        if(NOT DEFINED ${_variable})
            message(FATAL_ERROR
                "${fragment} does not set ${_variable}. Set it to an empty "
                "string if the platform has nothing to say (the contract is "
                "described at the top of cmake/Platform.cmake).")
        endif()
    endforeach()

    foreach(_hook IN LISTS SNS_PLATFORM_CONTRACT_HOOKS)
        if(NOT COMMAND ${_hook})
            message(FATAL_ERROR
                "${fragment} does not define ${_hook}(). Define it as a "
                "no-op if the platform has nothing to do.")
        endif()
    endforeach()

    if(NOT SNS_PLATFORM_DESKTOP STREQUAL "ON" AND NOT SNS_PLATFORM_DESKTOP STREQUAL "OFF")
        message(FATAL_ERROR
            "${fragment}: SNS_PLATFORM_DESKTOP must be ON or OFF, not "
            "'${SNS_PLATFORM_DESKTOP}'.")
    endif()

    if(NOT SNS_PLATFORM_EXECUTABLE_FORMAT MATCHES "^(elf|pe|mach-o|wasm)$")
        message(FATAL_ERROR
            "${fragment}: SNS_PLATFORM_EXECUTABLE_FORMAT must be elf, pe, mach-o or wasm, "
            "not '${SNS_PLATFORM_EXECUTABLE_FORMAT}'.")
    endif()

    # An empty one would install the application's dictionary into the install
    # prefix itself. The fragments derive it from CMAKE_INSTALL_BINDIR, which
    # cmake/Platform.cmake has included GNUInstallDirs for before reading the
    # fragment; this catches that include going missing.
    if(NOT SNS_PLATFORM_INSTALL_APP_DATA_DIR)
        message(FATAL_ERROR
            "${fragment}: SNS_PLATFORM_INSTALL_APP_DATA_DIR is empty. It is the "
            "directory the application's data is installed into, relative to the "
            "install prefix (usually CMAKE_INSTALL_BINDIR).")
    endif()

    if(NOT SNS_PLATFORM_QT_DEPLOY_AT_INSTALL STREQUAL "ON" AND
       NOT SNS_PLATFORM_QT_DEPLOY_AT_INSTALL STREQUAL "OFF")
        message(FATAL_ERROR
            "${fragment}: SNS_PLATFORM_QT_DEPLOY_AT_INSTALL must be ON or OFF, "
            "not '${SNS_PLATFORM_QT_DEPLOY_AT_INSTALL}'.")
    endif()

    foreach(_file IN LISTS SNS_PLATFORM_SOURCES)
        if(NOT EXISTS "${platforms_dir}/${_file}")
            message(FATAL_ERROR
                "${fragment} lists ${_file}, which does not exist under "
                "${platforms_dir}.")
        endif()
    endforeach()

    # Every interface header under interfaces/sns/platform must be implemented
    # by the fragment's own sources: a platform that compiles no definition of
    # vibrate(), say, links only by accident on a host where another target
    # happens to provide one. There is no way to see a C++ definition from
    # CMake, so the rule is that one of the fragment's sources includes the
    # header (every implementation does, as the compiler would demand of it).
    # It finds a new interface header without an implementation on every
    # platform, and the haptics interface added with no platform backend at
    # once.
    file(GLOB _interface_headers "${platforms_dir}/interfaces/sns/platform/*.hpp")
    foreach(_header IN LISTS _interface_headers)
        get_filename_component(_header_name "${_header}" NAME)
        set(_implemented FALSE)
        foreach(_source IN LISTS SNS_PLATFORM_SOURCES)
            if(_implemented)
                break()
            endif()
            file(READ "${platforms_dir}/${_source}" _source_text)
            string(FIND "${_source_text}" "#include \"sns/platform/${_header_name}\"" _include_at)
            if(NOT _include_at EQUAL -1)
                set(_implemented TRUE)
            endif()
        endforeach()
        if(NOT _implemented)
            message(FATAL_ERROR
                "${fragment} names no source that includes sns/platform/${_header_name}. "
                "Every interface in libs/platforms/interfaces/sns/platform/ needs one "
                "implementation per platform (add one, or the shared no-op under "
                "libs/platforms/detail/ and name it in the fragment).")
        endif()
    endforeach()
endfunction()
