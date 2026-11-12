# BeBoxTests: testing the BeOS R5 / BeBox target

Test programs for the `powerpc-be-beos` toolchain. They are built with the BeBox
compiler, copied to a BeOS machine (a real BeBox, or an emulator) and run there;
the output is compared with a stored expected output. They are registered with
`ctest` as `BeBox.<name>`.

If you only want to check that the toolchain works on your machine, read
"Quick start" and stop.

## What is tested

| Test | Covers |
|---|---|
| `hello` | C program, default link (`powerpc-be-beos-gcc hello.c`), `argc` |
| `stdio` | newlib stdio on libroot: `printf`, `fprintf(stderr)`, `fopen`/`fgets`/`remove`, `malloc` |
| `atexit` | `atexit` handlers run after `main` |
| `flush` | stdio is flushed at exit even without a final newline; handlers run in reverse order |
| `cxx_basic` | C++ global constructor and destructor, virtual calls, `new`/`delete`, `new[]` |
| `cxx_exceptions` | exceptions with stack unwinding, `std::string`, `std::vector` |

Not covered, because the target does not support it yet: `libbe` and GUI
programs, shared libraries and add-ons, application packaging (see the PR
description and `GCC-PATCHES.md`, section 18 and 22).

## What you need

1. **The toolchain, built with BeBox support**, and its `bin` directory on your `PATH`:

       mkdir Retro68-build && cd Retro68-build
       ../Retro68/build-toolchain.bash --bebox

   (add `--no-68k --no-ppc` to build only the BeBox part). This builds
   `powerpc-be-beos-gcc`/`g++`, the libraries, the import library for libroot
   (`libroot.a`, made from `ImportLibraries/BeOS/libroot.exports`), and the
   tests in `build-target-bebox/BeBoxTests/`.

2. **A BeOS R5 machine on the network** with the FTP and telnet servers enabled
   (Preferences, Network, FTP and Telnet), for example a BeBox. A user account
   with a password. The tests copy their programs to
   `/boot/home/retro68-tests/` (created if missing; the programs stay there so
   you can run them by hand) and run them with `bash`.

3. **Python 3** on the host (standard library only).

## Quick start

    export BEBOX_HOST=192.168.77.2        # address of the BeOS machine
    export BEBOX_USER=<login>
    export BEBOX_PASSWORD=<password>      # environment only, never a command line
    cd Retro68-build/build-target-bebox
    ctest

Expected: `100% tests passed out of 6` (about 5 seconds per test, mostly
telnet start-up). If the machine cannot be reached or no login is given, the
tests show as **Skipped**, not failed, so the suite is also safe to run on
machines without a BeBox.

Useful options:

    ctest -R cxx                          # only some tests
    ctest --output-on-failure             # show the program's output on failure
    cmake . -DBEBOX_TEST_HOST=10.0.0.5    # same as BEBOX_HOST, remembered in the build
    cmake . -DBEBOX_TEST_REMOTE_DIR=/boot/home/tmp

### With an emulator

Any emulator that runs BeOS R5 with networking works, as long as it forwards
the guest's FTP (21) and telnet (23) ports to the host:

    cmake . -DBEBOX_TEST_HOST=127.0.0.1 \
            -DBEBOX_TEST_FTP_PORT=2121 -DBEBOX_TEST_TELNET_PORT=2323
    ctest

### One program by hand

    python3 ../../BeBoxTests/run_on_beos.py \
        --binary BeBoxTests/hello.pef --expected ../../BeBoxTests/expected/hello.txt --show

`run_on_beos.py --help` lists all options. Exit status 0 is a pass, 1 a
failure (the output and the expected text are printed), 77 a skip.

## Adding a test

1. Put the source in `src/` and the exact expected output (standard output and
   standard error merged, as the program prints it) in `expected/<name>.txt`.
2. Add `add_beos_test(<name> <source>)` to `CMakeLists.txt`.
3. Run it on a real machine once and check the output by eye before storing it
   as the expected text.

Programs must be deterministic and must not read input. They run in
`/boot/home/retro68-tests/` with no arguments, so `argc` is 1.

## Troubleshooting

| Symptom | Cause |
|---|---|
| `cannot find -lroot` while building | `libroot.a` was not installed: run `build-toolchain.bash --bebox`, or `MAKEIMPORT_TOOL_PREFIX=powerpc-be-beos- MakeImport --from-list ImportLibraries/BeOS/libroot.exports libroot.so <prefix>/powerpc-be-beos/lib/libroot.a` |
| every test Skipped | no `BEBOX_USER`, or the machine is not reachable on FTP/telnet |
| `FAIL`, output is empty or the program is not found | the program was not marked executable, or the machine's FTP server is read-only for your account |
| the program does not start and the loader complains about an unresolved symbol | the import library lacks a function the program uses; add it to `ImportLibraries/BeOS/libroot.exports` (only names that `libroot.so` exports) |
| telnet times out | raise `--timeout` (first telnet login on a BeBox can take several seconds) |

## The same kind of test on the other Retro68 platforms

Retro68's other targets have a test suite too, `AutomatedTests/`. It works the
same way (build, run, compare) but starts classic Mac programs through
`LaunchAPPL`:

| Platform | Directory | Build directory | Run | What runs the program |
|---|---|---|---|---|
| 68K Mac | `AutomatedTests/` | `build-target` | `ctest` | `LaunchAPPL` with an emulator (`classic`) |
| classic PowerPC Mac | `AutomatedTests/` | `build-target-ppc` | `ctest` | `LaunchAPPL` |
| Carbon | `AutomatedTests/` | `build-target-carbon` | `ctest` | `LaunchAPPL` (`carbon`) |
| BeOS / BeBox | `BeBoxTests/` | `build-target-bebox` | `ctest` | `run_on_beos.py` (FTP + telnet) |

Test names carry the platform as a prefix (`M68K.`, `PPC.`, `Carbon.`,
`BeBox.`), so a combined run can be filtered with `ctest -R '^BeBox\.'`.
The Mac suites are described in the main `README.md`, "The Test Suite".
