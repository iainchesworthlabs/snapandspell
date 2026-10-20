# ---------------------------------------------------------------------------
# A trace of the program really holds the names the documents say it does.
#
#   cmake -DCASE=<cli|app> -DTRACY_TOOLS=<directory of tracy-capture> -DPROGRAM=<exe>
#         -DWORK_DIR=<dir> -DPORT=<n> [-DEXE_SUFFIX=<.exe>] -P check_trace.cmake
#
# check_zone_names.cmake reads the sources: it cannot tell a zone in code that
# never runs, a name that was renamed in one place, or a plot whose signal the
# window never emits. This runs the program with the profiler attached, exports
# the trace and asserts that the zones, plots and frame marks a trace of that
# program is made of are in it. Only a build with Tracy on can do it, and only
# where the capture tools were built (they are part of the same vcpkg feature),
# so everywhere else it reports itself skipped (exit 4, the test's
# SKIP_RETURN_CODE) with the reason.
#
# The client listens on a port of its own for each run (TRACY_PORT, and the
# capture's -p): every Tracy client listens on 8086 by default, so a capture
# started with only an address attaches to whichever program on the machine
# started first, a sibling project's included, and the smoke's own wait for a
# profiler can bind to the wrong program (docs/profiling.md, "Which program the
# capture attaches to").
#
# The two programs run for this long:
#   cli   `wjs-cli --repeat N`: solves a board N times. A capture takes about 0.7 s
#         to find a program (Tracy is on-demand), and the first solves are over by
#         then, so the board is solved enough times for the capture to be inside it.
#   app   `WordJewelsSolver --smoke`: waits up to five seconds for a profiler before
#         loading anything, so the start-up zones are in the capture too.
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.28)

foreach(_required IN ITEMS CASE WORK_DIR PORT PROGRAM)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "check_trace.cmake: ${_required} is required")
    endif()
endforeach()

# --- what each program's trace is made of -----------------------------------------
# Names are the ones the code emits at run time. A name added here is one the
# program must emit; one removed is one the program no longer does.
if(CASE STREQUAL "cli")
    set(_args "S E R S/P A T G/L I N E/S E R S")
    # More work each time the capture was too late for the one before.
    set(_repeats 100 1000 8000)
    set(_zones solver.solve)
    set(_plots solver.visits)
    set(_timeout 30)
elseif(CASE STREQUAL "app")
    # The offscreen platform: Quick 3D draws nothing there but the frame clock
    # and the start-up are the same (docs/profiling.md, "The smoke mode").
    set(_args --smoke 120 --store "${WORK_DIR}/app-store.wjs")
    set(_repeats 0)
    # The QML stage zones are in it (BoardScene.stageZone() chooses them, so only
    # a run can show they are emitted), and the frame's plots. render.record_ms is
    # not asked for: the offscreen platform draws with a software renderer that has
    # no render pass to record, so the window never emits the signals it is made from.
    set(_zones
        app.startup app.load_qml data.load_dictionary data.load_store data.save_store
        board.deal board.roll solver.solve
        scene.step scene.prepare scene.play scene.show scene.shake scene.drop scene.settle)
    set(_plots
        solver.visits ui.frame_ms ui.first_frame_ms ui.sync_ms ui.animate_ms render.frame_ms)
    set(_timeout 180)
else()
    message(FATAL_ERROR "check_trace.cmake: unknown CASE '${CASE}'")
endif()

# --- can it run here -----------------------------------------------------------------
if(NOT DEFINED EXE_SUFFIX)
    set(EXE_SUFFIX "")
endif()
set(_capture "${TRACY_TOOLS}/tracy-capture${EXE_SUFFIX}")
set(_export "${TRACY_TOOLS}/tracy-csvexport${EXE_SUFFIX}")
if("${TRACY_TOOLS}" STREQUAL "" OR NOT EXISTS "${_capture}" OR NOT EXISTS "${_export}")
    message(STATUS
        "SKIPPED: this build has no Tracy capture tools (WJS_ENABLE_TRACY is off, or "
        "'${TRACY_TOOLS}' does not hold tracy-capture and tracy-csvexport). A Debug or "
        "*-profiling build has them; docs/profiling.md, \"Building a profiling binary\".")
    cmake_language(EXIT 4)
endif()
if(NOT EXISTS "${PROGRAM}")
    message(STATUS "SKIPPED: '${PROGRAM}' was not built in this configuration.")
    cmake_language(EXIT 4)
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")
set(_trace "${WORK_DIR}/${CASE}.tracy")
file(REMOVE "${_trace}")

set(ENV{TRACY_PORT} "${PORT}")
if(CASE STREQUAL "app")
    set(ENV{QT_QPA_PLATFORM} "offscreen")
    set(ENV{QT_FORCE_STDERR_LOGGING} "1")
endif()

# The capture and the program start together and run side by side: execute_process
# runs the commands of one call at once, joined by a pipe. The capture runs through
# run_capture.cmake, which writes nothing to the pipe (a capture that does is killed
# by SIGPIPE on macOS when the program has gone, before it saves the trace) and
# stops after 20 s whether or not the program is done. It waits for the program to
# appear and stops when it exits.
#
# A capture takes about 0.7 s to find a program (Tracy is on-demand), and a program
# that is quick (an optimised build) can be over by then, so the cli is run again
# with more repeats until the trace has what it is asked for (30 s an attempt). A
# slow build (Debug with the sanitizers) is done in the first.
set(_attempt 0)
list(LENGTH _repeats _attempts)
foreach(_repeat IN LISTS _repeats)
    math(EXPR _attempt "${_attempt} + 1")
    set(_run_args ${_args})
    if(CASE STREQUAL "cli")
        set(_run_args --repeat ${_repeat} ${_args})
    endif()
    file(REMOVE "${_trace}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DCAPTURE=${_capture}" "-DTRACE=${_trace}"
            "-DPORT=${PORT}" -DSECONDS=20 -P "${CMAKE_CURRENT_LIST_DIR}/run_capture.cmake"
        COMMAND "${PROGRAM}" ${_run_args}
        RESULTS_VARIABLE _codes
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err
        TIMEOUT ${_timeout})

    # The app says so when it cannot present a frame (exit 3): the offscreen platform
    # did not give Quick a window here, which is a different machine's answer.
    # A program that was over before the capture attached leaves the capture waiting
    # for one that is not coming; the timeout ends the attempt, and the next has more
    # to do.
    if("${_codes}" MATCHES "timeout")
        if(_attempt LESS _attempts)
            message(STATUS "attempt ${_attempt} of ${_attempts} timed out; running again")
            continue()
        endif()
        message(FATAL_ERROR
            "${PROGRAM} did not finish in ${_timeout} s with the capture attached, in any of "
            "${_attempts} attempts (${_repeats} repeats).\nstdout:\n${_out}\nstderr:\n${_err}")
    endif()
    list(GET _codes 1 _program_code)
    if(CASE STREQUAL "app" AND _program_code EQUAL 3)
        message(STATUS "SKIPPED: the window presented no frame on this platform:\n${_err}")
        cmake_language(EXIT 4)
    endif()
    if(NOT _program_code EQUAL 0)
        message(FATAL_ERROR
            "${PROGRAM} exited with ${_program_code} while being captured (exit codes "
            "${_codes}).\nstdout:\n${_out}\nstderr:\n${_err}")
    endif()
    if(NOT EXISTS "${_trace}")
        if(_attempt LESS _attempts)
            continue()
        endif()
        message(FATAL_ERROR
            "No trace was captured from ${PROGRAM} (exit codes ${_codes}) on port ${PORT}.\n"
            "stdout:\n${_out}\nstderr:\n${_err}")
    endif()

    # --- read the trace ----------------------------------------------------------------
    # One row a zone: name,src_file,src_line,total_ns,total_perc,counts,...
    execute_process(
        COMMAND "${_export}" "${_trace}"
        RESULT_VARIABLE _export_code
        OUTPUT_VARIABLE _zone_csv
        ERROR_VARIABLE _export_err)
    if(NOT _export_code EQUAL 0)
        message(FATAL_ERROR "tracy-csvexport failed on ${_trace}: ${_export_err}")
    endif()
    # One row a plot sample (-u -p): the plot's name first. The filter is for the zone
    # events -u lists, of which there are millions (a zone a scored word); a name that
    # no zone has leaves the plots, and the "Frame" rows of the frame marks, which are
    # not plots.
    execute_process(
        COMMAND "${_export}" -u -p -f "no zone is named this" "${_trace}"
        RESULT_VARIABLE _export_code
        OUTPUT_VARIABLE _plot_csv
        ERROR_VARIABLE _export_err)
    if(NOT _export_code EQUAL 0)
        message(FATAL_ERROR "tracy-csvexport -u -p failed on ${_trace}: ${_export_err}")
    endif()

    # The names the trace holds, each with how many times it ran (zones) or was sampled.
    set(_seen_zones "")
    string(REPLACE "\n" ";" _zone_lines "${_zone_csv}")
    foreach(_line IN LISTS _zone_lines)
        string(REPLACE "\r" "" _line "${_line}")
        if("${_line}" STREQUAL "")
            continue()
        endif()
        string(REPLACE "," ";" _fields "${_line}")
        list(GET _fields 0 _name)
        list(LENGTH _fields _count_of_fields)
        if(_count_of_fields GREATER 5)
            list(GET _fields 5 _count)
            if(_count MATCHES "^[0-9]+$" AND _count GREATER 0)
                list(APPEND _seen_zones "${_name}")
            endif()
        endif()
    endforeach()

    set(_seen_plots "")
    string(REPLACE "\n" ";" _plot_lines "${_plot_csv}")
    foreach(_line IN LISTS _plot_lines)
        string(REPLACE "\r" "" _line "${_line}")
        if("${_line}" STREQUAL "")
            continue()
        endif()
        string(REPLACE "," ";" _fields "${_line}")
        list(GET _fields 0 _name)
        if(NOT _name STREQUAL "name" AND NOT _name STREQUAL "Frame")
            list(APPEND _seen_plots "${_name}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _seen_plots)

    set(_failures "")
    foreach(_name IN LISTS _zones)
        if(NOT _name IN_LIST _seen_zones)
            string(APPEND _failures "\n  the zone '${_name}' is not in the trace")
        endif()
    endforeach()
    foreach(_name IN LISTS _plots)
        if(NOT _name IN_LIST _seen_plots)
            string(APPEND _failures "\n  the plot '${_name}' has no sample in the trace")
        endif()
    endforeach()
    list(LENGTH _seen_zones _n_zones)
    list(LENGTH _seen_plots _n_plots)
    if(NOT _failures)
        message(STATUS
            "OK: the trace of ${CASE} holds ${_n_zones} zones and ${_n_plots} plots, including "
            "all of ${_zones} and ${_plots}")
        break()
    endif()
    if(_attempt LESS _attempts)
        message(STATUS "attempt ${_attempt} of ${_attempts} was over before the capture "
                       "attached (${_failures}); running again with more work")
        continue()
    endif()
    message(FATAL_ERROR
        "The trace of ${CASE} (${_trace}) is missing names a trace of it is made of:${_failures}\n"
        "It holds ${_n_zones} zones: ${_seen_zones}\nand ${_n_plots} plots: ${_seen_plots}")
endforeach()
