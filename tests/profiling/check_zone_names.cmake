# ---------------------------------------------------------------------------
# Every zone has a name of the right shape, no name is used twice, and the
# zones a trace of the application's work is made of exist.
#
#   cmake -DREPO_ROOT=<repository root> -P tests/profiling/check_zone_names.cmake
#
# A zone's source location is a function-local static of zone<"name">() (see
# src/lib/variants/profiling-enabled/wjs/profiling.hpp), so there is one per
# NAME: a name used at two places shows the first one's file and line, and the
# second's call is indistinguishable from it in a trace. The names are also
# what the CSV export and the profiler show and what any baseline reads, so
# they are `<layer>.<thing>`, lower case, dotted (docs/profiling.md).
#
# The names that are not zones have their own shapes and are checked too:
# frame sets are a plain name ("UI", "Capture"), thread names are lower case
# ("main", "render"), and plots are `<layer>.<thing>` as zones are. A plot
# used twice is checked, because two plots of one name are one line.
#
# The QML seam's names come from QML, not from a template argument, so they are
# found as text: `Profiler.begin("name")` and `Profiler.mark("name")` in the
# application's `.qml` and `.js` files. They share the C++ zones' namespace: a
# QML name may not also be a C++ zone's, because the cache would then make one
# name two places. A QML name may be used at more than one call site: the seam
# caches one source location for each name, which is what one name one place
# means there (the calls are one zone, whatever opens them).
#
# Scans src/ for `zone<"...">`, `plot("...")`, `frame_mark("...")`,
# `thread_name("...")`, and the QML files for the Profiler calls. The variant
# headers and profiling_types.hpp are skipped (they are where the calls are
# defined, and their comments quote them), and so is every line that is a
# // comment.
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.28)

if(NOT REPO_ROOT)
    message(FATAL_ERROR "Pass -DREPO_ROOT=<repository root>.")
endif()

set(_zone_shape "^[a-z][a-z0-9_]*(\\.[a-z][a-z0-9_]*)+$")
set(_frame_shape "^[A-Z][A-Za-z0-9]*$")
set(_thread_shape "^[a-z][a-z0-9_-]*$")

set(_failures "")
function(_fail text)
    set(_failures "${_failures}\n  ${text}" PARENT_SCOPE)
endfunction()

set(_zone_names "")
set(_qml_names "")
set(_plot_names "")
set(_frame_names "")
set(_thread_names "")
set(_zones 0)
set(_qml_scopes 0)
set(_plots 0)
set(_frames 0)
set(_threads 0)

# --- the C++ calls ------------------------------------------------------------

file(GLOB_RECURSE _sources
    "${REPO_ROOT}/src/*.cpp" "${REPO_ROOT}/src/*.hpp" "${REPO_ROOT}/src/*.mm")

foreach(_source IN LISTS _sources)
    file(RELATIVE_PATH _relative "${REPO_ROOT}" "${_source}")
    if(_relative MATCHES "^src/lib/variants/" OR _relative STREQUAL "src/lib/include/wjs/profiling_types.hpp")
        continue()
    endif()

    file(STRINGS "${_source}" _lines)
    set(_number 0)
    foreach(_line IN LISTS _lines)
        math(EXPR _number "${_number} + 1")
        if(_line MATCHES "^[ \t]*//")
            continue()
        endif()
        set(_where "${_relative}:${_number}")

        if(_line MATCHES "zone<\"([^\"]*)\">")
            set(_name "${CMAKE_MATCH_1}")
            math(EXPR _zones "${_zones} + 1")
            if(NOT _name MATCHES "${_zone_shape}")
                _fail("${_where}: zone '${_name}' is not <layer>.<thing>, lower case and dotted")
            endif()
            if("${_name}" IN_LIST _zone_names)
                _fail("${_where}: zone '${_name}' is used again; one name is one place")
            endif()
            list(APPEND _zone_names "${_name}")
        elseif(_line MATCHES "plot[ \t]*\\(\"([^\"]*)\"")
            set(_name "${CMAKE_MATCH_1}")
            math(EXPR _plots "${_plots} + 1")
            if(NOT _name MATCHES "${_zone_shape}")
                _fail("${_where}: plot '${_name}' is not <layer>.<thing>, lower case and dotted")
            endif()
            if("${_name}" IN_LIST _plot_names)
                _fail("${_where}: plot '${_name}' is used again; one line is one name")
            endif()
            list(APPEND _plot_names "${_name}")
        elseif(_line MATCHES "frame_mark[ \t]*\\(\"([^\"]*)\"")
            set(_name "${CMAKE_MATCH_1}")
            math(EXPR _frames "${_frames} + 1")
            if(NOT _name MATCHES "${_frame_shape}")
                _fail("${_where}: frame set '${_name}' is not a plain name")
            endif()
            if("${_name}" IN_LIST _frame_names)
                _fail("${_where}: frame set '${_name}' is used again")
            endif()
            list(APPEND _frame_names "${_name}")
        elseif(_line MATCHES "thread_name[ \t]*\\(\"([^\"]*)\"")
            set(_name "${CMAKE_MATCH_1}")
            math(EXPR _threads "${_threads} + 1")
            if(NOT _name MATCHES "${_thread_shape}")
                _fail("${_where}: thread name '${_name}' is not lower case")
            endif()
            # Not checked for repeats: "main" is the main thread of every
            # program in the tree, and each is the one thread of its program.
            list(APPEND _thread_names "${_name}")
        endif()
    endforeach()
endforeach()

# --- the QML seam -------------------------------------------------------------

file(GLOB_RECURSE _qml_sources "${REPO_ROOT}/src/*.qml" "${REPO_ROOT}/src/*.js")
foreach(_source IN LISTS _qml_sources)
    file(RELATIVE_PATH _relative "${REPO_ROOT}" "${_source}")
    file(STRINGS "${_source}" _lines)
    set(_number 0)
    foreach(_line IN LISTS _lines)
        math(EXPR _number "${_number} + 1")
        if(_line MATCHES "^[ \t]*//")
            continue()
        endif()
        if(NOT _line MATCHES "Profiler[ \t]*\\.[ \t]*(begin|mark)[ \t]*\\([ \t]*\"([^\"]*)\"")
            continue()
        endif()
        set(_kind "${CMAKE_MATCH_1}")
        set(_name "${CMAKE_MATCH_2}")
        set(_where "${_relative}:${_number} (Profiler.${_kind})")
        math(EXPR _qml_scopes "${_qml_scopes} + 1")
        if(NOT _name MATCHES "${_zone_shape}")
            _fail("${_where}: '${_name}' is not <layer>.<thing>, lower case and dotted")
        endif()
        if("${_name}" IN_LIST _zone_names)
            _fail("${_where}: '${_name}' is also a C++ zone; one name is one place")
        endif()
        list(APPEND _qml_names "${_name}")
    endforeach()
endforeach()

# --- the required names --------------------------------------------------------
# "Profiling everywhere" is held here, not by convention: the zones, plots and
# QML scopes a trace of the application's work is made of must exist, so
# deleting an instrumented call site is a failed test, as deleting a unit test
# is. A name is added here with the code it times, and removed only with a
# reason in the commit that says where the work went.
set(_required_zones
    app.load_qml
    app.startup
    board.deal
    board.deal_playable
    board.parse
    board.restore
    board.roll
    board.seed
    board.verify
    camera.convert_frame
    cli.run
    data.build_dashboard
    data.load_catalogue
    data.load_dictionary
    data.load_phase
    data.load_store
    data.save_store
    dice.load_sets
    feedback.plan
    gate.refresh
    input.submit
    input.trace
    motion.push
    path.validate
    phonics.bind
    phonics.sound_it_out
    rules.classify
    scan.convert_working
    scan.decode_frame
    scan.find_blobs
    scan.find_board
    scan.process_frame
    scan.read_glyphs
    scan.read_tiles
    scan.warp_board
    scene.step
    session.finish
    settings.normalize
    shake.detect
    solo.blocks
    solo.history
    solo.path
    solo.submit
    solver.model
    solver.solve
    store.parse
    store.serialize
    store.session_record
    store.session_snapshot)
set(_required_plots
    render.frame_ms
    solver.visits
    ui.first_frame_ms
    ui.frame_ms
    ui.sync_ms)
set(_required_qml
    scene.play
    scene.prepare
    scene.show
    scene2d.play
    scene2d.show
    scene2d.step
    ui.screen
    ui.trace)
foreach(_name IN LISTS _required_zones)
    if(NOT _name IN_LIST _zone_names)
        _fail("the required zone '${_name}' is gone; instrumentation was removed")
    endif()
endforeach()
foreach(_name IN LISTS _required_plots)
    if(NOT _name IN_LIST _plot_names)
        _fail("the required plot '${_name}' is gone")
    endif()
endforeach()
foreach(_name IN LISTS _required_qml)
    if(NOT _name IN_LIST _qml_names)
        _fail("the required QML scope '${_name}' is gone")
    endif()
endforeach()

# --- result -------------------------------------------------------------------

if(_failures)
    message(FATAL_ERROR "Profiling names:${_failures}")
endif()
list(REMOVE_DUPLICATES _qml_names)
list(LENGTH _qml_names _qml_named)
message(STATUS
    "OK: ${_zones} C++ zones, ${_qml_scopes} QML scopes (${_qml_named} names), ${_plots} plots, "
    "${_frames} frame sets, ${_threads} thread names, each named once")
