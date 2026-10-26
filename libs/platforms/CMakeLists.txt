# The platform library: one implementation of every interfaces/ header, for the
# platform cmake/Platform.cmake selected. Which files those are, and what they
# link against, is declared by that platform's platform.cmake; nothing here
# asks which operating system this is.
add_library(sns_platform STATIC)
add_library(sns::platform ALIAS sns_platform)

set(_sns_platform_sources "")
foreach(_source IN LISTS SNS_PLATFORM_SOURCES)
    list(APPEND _sns_platform_sources "${CMAKE_CURRENT_SOURCE_DIR}/${_source}")
endforeach()

target_sources(sns_platform PRIVATE ${_sns_platform_sources})
target_include_directories(sns_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interfaces")
target_compile_features(sns_platform PUBLIC cxx_std_23)
# The camera and board_reader interfaces are written in terms of the scanner's
# frame and reading types, so the platform layer sits on sns_scan (which sits on
# the game library, and includes no platform header).
target_link_libraries(sns_platform PUBLIC sns_scan)
target_compile_definitions(sns_platform PRIVATE ${SNS_PLATFORM_COMPILE_DEFINITIONS})
target_link_libraries(sns_platform
    PRIVATE
        sns::warnings
        sns::hardening
        sns::sanitizers
        sns::coverage
        ${SNS_PLATFORM_LINK_LIBRARIES})
