/*
    This file is part of Retro68.

    Retro68 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Retro68 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Under Section 7 of GPL version 3, you are granted additional
    permissions described in the GCC Runtime Library Exception, version
    3.1, as published by the Free Software Foundation.

    You should have received a copy of the GNU General Public License and
    a copy of the GCC Runtime Library Exception along with this program;
    see the files COPYING and COPYING.RUNTIME respectively.  If not, see
    <http://www.gnu.org/licenses/>.
*/

/*
    Program entry point for BeOS R5 (PowerPC / BeBox) executables built with
    GCC. It does what the Metrowerks `start_dyn.o` that ships with BeOS does:
    the loader calls __start(argc, argv, envp), and the code

      - saves argv and envp (falling back to a one-entry default environment
        if the loader passed none),
      - records the main thread's ID,
      - runs the C runtime's init routines (_call_init_routines_, in libroot),
      - calls main(), tells the kernel the thread is done, runs the atexit
        handlers (C++ global destructors) and flushes stdio, and calls exit().

    The behaviour and the exported data symbols (argv_save, environ,
    __main_thread_id) were taken from a disassembly of start_dyn.o (see
    notes/retro68-toolchain-plan.md in the BeBox project); the source here is
    written from scratch.

    Build notes:
      - argv_save, environ and __main_thread_id are statically initialised in
        Be's object (data section, not bss), and other code may expect that.
        Compile this file with -fno-zero-initialized-in-bss so GCC does the
        same.
      - Metrowerks' fragment init/term hooks (init_term_dyn.o) are separate;
        see beosinitterm.c (not written yet).
*/

#include <stdio.h>

typedef int thread_id;

/* newlib's atexit machinery. Linking against libroot makes exit() resolve to
   libroot's, which knows nothing about newlib's atexit list, so __start runs
   the handlers itself. */
extern void __call_exitprocs(int status, void *dso);

extern int main(int argc, char **argv, char **envp);
extern void exit(int status) __attribute__((noreturn));

/* libroot */
extern thread_id find_thread(const char *name);
extern void _call_init_routines_(void);
extern void _thread_do_exit_notification(void);

char **argv_save = 0;
char **environ = 0;
thread_id __main_thread_id = 0;

/* Used when the loader passes no environment. */
static char *default_environ[] = { "NO_ENVIRON=true", 0 };

/* Present in Be's start_dyn.o ("!TD!" + 4 0 0 0); nothing in it refers to it. */
const char magic_template[8] = { '!', 'T', 'D', '!', 4, 0, 0, 0 };

void __start(int argc, char **argv, char **envp)
{
    argv_save = argv;
    environ = envp;
    if (!envp)
        environ = default_environ;

    __main_thread_id = find_thread(0);
    _call_init_routines_();

    {
        int status = main(argc, argv, envp);
        _thread_do_exit_notification();
        __call_exitprocs(status, 0);
        fflush(NULL);
        exit(status);
    }
}
