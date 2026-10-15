# The platform library: one implementation of every interfaces/ header, for the
# platform cmake/Platform.cmake selected. Which files those are, and what they
# link against, is declared by that platform's platform.cmake; nothing here
# asks which operating system this is.
add_library(wjs_platform STATIC)
add_library(wjs::platform ALIAS wjs_platform)

set(_wjs_platform_sources "")
foreach(_source IN LISTS WJS_PLATFORM_SOURCES)
    list(APPEND _wjs_platform_sources "${CMAKE_CURRENT_SOURCE_DIR}/${_source}")
endforeach()

target_sources(wjs_platform PRIVATE ${_wjs_platform_sources})
target_include_directories(wjs_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/interfaces")
target_compile_features(wjs_platform PUBLIC cxx_std_23)
target_compile_definitions(wjs_platform PRIVATE ${WJS_PLATFORM_COMPILE_DEFINITIONS})
target_link_libraries(wjs_platform
    PRIVATE
        wjs::warnings
        wjs::sanitizers
        wjs::coverage
        ${WJS_PLATFORM_LINK_LIBRARIES})
