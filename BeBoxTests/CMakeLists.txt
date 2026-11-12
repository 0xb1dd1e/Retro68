#   This file is part of Retro68.
#
#   Retro68 is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Retro68 is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Retro68.  If not, see <http://www.gnu.org/licenses/>.

# Test programs for the BeOS R5 / BeBox target. They are built here and run on
# a BeOS machine over FTP and telnet by run_on_beos.py (see README.md), as
# ctest cases named "BeBox.<test>". The tests are skipped, not failed, when no
# BeOS machine can be reached.

enable_testing()

find_program(BEBOX_TEST_PYTHON NAMES python3 python
    NO_CMAKE_FIND_ROOT_PATH)

set(BEBOX_TEST_HOST "$ENV{BEBOX_HOST}" CACHE STRING
    "Address of the BeOS machine that runs the tests (default: 192.168.77.2)")
set(BEBOX_TEST_FTP_PORT 21 CACHE STRING "FTP port of the BeOS machine (an emulator may forward another port)")
set(BEBOX_TEST_TELNET_PORT 23 CACHE STRING "Telnet port of the BeOS machine")
set(BEBOX_TEST_REMOTE_DIR "/boot/home/retro68-tests" CACHE STRING
    "Directory on the BeOS machine that the test programs are copied to")

# add_beos_test(<name> <source> [expected-output-file])
# Builds <source> into <name>.pef and registers the ctest case BeBox.<name>.
function(add_beos_test name source)
    add_executable(${name} src/${source})
    # the ld output is an XCOFF file; MakePEF turns it into a BeOS executable
    add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${MAKE_PEF} $<TARGET_FILE:${name}> -o ${CMAKE_CURRENT_BINARY_DIR}/${name}.pef
        COMMENT "MakePEF ${name}")
    if(BEBOX_TEST_PYTHON)
        set(host_args)
        if(BEBOX_TEST_HOST)
            set(host_args --host ${BEBOX_TEST_HOST})
        endif()
        add_test(NAME BeBox.${name}
            COMMAND ${BEBOX_TEST_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/run_on_beos.py
                --binary ${CMAKE_CURRENT_BINARY_DIR}/${name}.pef
                --expected ${CMAKE_CURRENT_SOURCE_DIR}/expected/${name}.txt
                --name ${name} --remote-dir ${BEBOX_TEST_REMOTE_DIR}
                --ftp-port ${BEBOX_TEST_FTP_PORT} --telnet-port ${BEBOX_TEST_TELNET_PORT} ${host_args})
        set_tests_properties(BeBox.${name} PROPERTIES SKIP_RETURN_CODE 77)
    endif()
endfunction()

add_beos_test(hello          hello.c)           # C, default link, argc
add_beos_test(stdio          stdio.c)           # printf, stderr, fopen/fgets/remove, malloc
add_beos_test(atexit         atexit.c)          # atexit handlers run after main
add_beos_test(flush          flush.c)           # stdio flushed at exit, output without newline
add_beos_test(cxx_basic      cxx_basic.cc)      # global ctor/dtor, virtual calls, new/delete
add_beos_test(cxx_exceptions cxx_exceptions.cc) # exceptions with unwinding, std::string, std::vector
