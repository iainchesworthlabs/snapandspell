# SPDX-License-Identifier: GPL-3.0-or-later
# What the hardening flags (cmake/Hardening.cmake) did to a built program, read back from the
# ELF file: a position-independent executable, a read-only relocation segment with immediate
# binding and a stack protector in the code. On x86-64 the control-flow protection note is read
# from a static library of ours instead: the linker keeps the note only when every input has it,
# and a distribution's startup files (Debian's crt1.o) may not, which says nothing about our code.
# A flag that a compiler quietly ignored, or a preset that stopped passing, fails here and not
# in a security review.
#
# Run as:
#   cmake -DPROGRAM=<path to an ELF executable> -DREADELF=<readelf> \
#         -DARCH=<CMAKE_SYSTEM_PROCESSOR> -DARCHIVE=<path to a static library of ours> \
#         -P tests/hardening/check_binary.cmake

if(NOT DEFINED PROGRAM OR NOT DEFINED READELF)
    message(FATAL_ERROR "check_binary.cmake: pass -DPROGRAM=<executable> and -DREADELF=<readelf>")
endif()
if(NOT EXISTS "${PROGRAM}")
    message(FATAL_ERROR "check_binary.cmake: ${PROGRAM} does not exist; build it first")
endif()

set(_failures "")
function(_fail message_text)
    set(_failures "${_failures}\n  ${message_text}" PARENT_SCOPE)
endfunction()

function(_readelf_of file out_var)
    execute_process(
        COMMAND "${READELF}" ${ARGN} "${file}"
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE _err
        RESULT_VARIABLE _rc)
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "check_binary.cmake: readelf ${ARGN} failed on ${file}: ${_err}")
    endif()
    set(${out_var} "${_out}" PARENT_SCOPE)
endfunction()

function(_readelf out_var)
    _readelf_of("${PROGRAM}" _text ${ARGN})
    set(${out_var} "${_text}" PARENT_SCOPE)
endfunction()

_readelf(_header --file-header --wide)
if(NOT _header MATCHES "Type:[ ]+DYN")
    _fail("the program is not position independent (ELF type is not DYN)")
endif()

_readelf(_segments --program-headers --wide)
if(NOT _segments MATCHES "GNU_RELRO")
    _fail("the program has no GNU_RELRO segment (-Wl,-z,relro)")
endif()

_readelf(_dynamic --dynamic --wide)
if(NOT _dynamic MATCHES "BIND_NOW" AND NOT _dynamic MATCHES "FLAGS_1[^\n]*NOW")
    _fail("the program binds lazily (-Wl,-z,now is missing)")
endif()

_readelf(_symbols --dyn-syms --wide)
if(NOT _symbols MATCHES "__stack_chk_fail")
    _fail("nothing in the program calls __stack_chk_fail (-fstack-protector-strong did not apply)")
endif()

if(DEFINED ARCH AND ARCH MATCHES "x86_64|AMD64")
    if(NOT DEFINED ARCHIVE OR NOT EXISTS "${ARCHIVE}")
        message(FATAL_ERROR "check_binary.cmake: pass -DARCHIVE=<a static library of ours> on x86-64")
    endif()
    _readelf_of("${ARCHIVE}" _notes --notes --wide)
    if(NOT _notes MATCHES "IBT" OR NOT _notes MATCHES "SHSTK")
        _fail("${ARCHIVE} does not carry the IBT and SHSTK property (-fcf-protection=full)")
    endif()
endif()

if(_failures)
    message(FATAL_ERROR "${PROGRAM} is not hardened:${_failures}")
endif()
message(STATUS "hardening: ${PROGRAM} is a PIE with relro, immediate binding and a stack protector")
