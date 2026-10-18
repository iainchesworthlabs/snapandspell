# Runs qmllint over the application's QML with no warning allowed. Qt writes the
# list of files and the places to look for the types they use (a response file) when
# the application is configured, and the types themselves when it is built, so this
# runs after a build and reports itself skipped (exit 4) when there is none.
#
# The board scene is a QML module of its own (WordJewelsSolver.Scene), and so is
# the Family Assistant (WordJewelsSolver.Scan), so their response files and types
# are linted too, in the same case: a warning in either module fails the
# application's lint. The scan module's screens import the application's own
# module for its kit, and the application's shell imports the scan module.
#
# Run as: cmake -DQMLLINT=<qmllint> -DRESPONSE=<file.rsp> -DTYPES=<generated qmltypes>
#               [-DSCENE_RESPONSE=<file.rsp> -DSCENE_TYPES=<generated qmltypes>]
#               [-DSCAN_RESPONSE=<file.rsp> -DSCAN_TYPES=<generated qmltypes>]
#               -P run_qmllint.cmake

if(NOT EXISTS "${RESPONSE}" OR NOT EXISTS "${TYPES}")
    message(STATUS "the application has not been built: nothing to lint")
    cmake_language(EXIT 4)
endif()

set(_modules "${RESPONSE}|${TYPES}")
if(DEFINED SCENE_RESPONSE AND DEFINED SCENE_TYPES)
    list(APPEND _modules "${SCENE_RESPONSE}|${SCENE_TYPES}")
endif()
if(DEFINED SCAN_RESPONSE AND DEFINED SCAN_TYPES)
    list(APPEND _modules "${SCAN_RESPONSE}|${SCAN_TYPES}")
endif()

foreach(_module IN LISTS _modules)
    string(REPLACE "|" ";" _paths "${_module}")
    list(GET _paths 0 _response)
    list(GET _paths 1 _types)
    if(NOT EXISTS "${_response}" OR NOT EXISTS "${_types}")
        message(STATUS "a module has not been built: ${_response}")
        cmake_language(EXIT 4)
    endif()
    execute_process(
        COMMAND "${QMLLINT}" --max-warnings 0 "@${_response}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _errors)
    if(NOT _result EQUAL 0)
        message("${_output}${_errors}")
        message(FATAL_ERROR "qmllint found a problem in the application's QML: ${_response}")
    endif()
endforeach()
message(STATUS "qmllint: no warnings")
