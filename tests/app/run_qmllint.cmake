# Runs qmllint over the application's QML with no warning allowed. Qt writes the
# list of files and the places to look for the types they use (a response file) when
# the application is configured, and the types themselves when it is built, so this
# runs after a build and reports itself skipped (exit 4) when there is none.
#
# Run as: cmake -DQMLLINT=<qmllint> -DRESPONSE=<file.rsp> -DTYPES=<generated qmltypes>
#               -P run_qmllint.cmake

if(NOT EXISTS "${RESPONSE}" OR NOT EXISTS "${TYPES}")
    message(STATUS "the application has not been built: nothing to lint")
    cmake_language(EXIT 4)
endif()

execute_process(
    COMMAND "${QMLLINT}" --max-warnings 0 "@${RESPONSE}"
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _output
    ERROR_VARIABLE _errors)

if(NOT _result EQUAL 0)
    message("${_output}${_errors}")
    message(FATAL_ERROR "qmllint found a problem in the application's QML")
endif()
message(STATUS "qmllint: no warnings")
