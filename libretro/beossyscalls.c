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
    newlib's system call layer for BeOS R5 (BeBox), implemented on libroot.

    newlib's libc.a calls reentrant stubs (_write_r, _read_r, ...) that every
    port supplies. They are implemented here with libroot's own POSIX
    functions.

    Why the functions are looked up at run time: newlib's libc.a *also*
    defines write(), read(), close(), lseek(), isatty(), getpid(), kill(),
    open(), ... as thin wrappers that call the _r stubs below, and libroot
    exports the same names. If this file called libroot's write() by name the
    linker would pick newlib's write(), which calls _write_r(), which is us
    again: endless recursion. get_image_symbol() finds libroot's real
    functions instead, whichever library the linker preferred.

    Translations:
      - errno: libroot sets BeOS error codes (offsets from LONG_MIN); the
        result is converted to newlib's numbers in the reent structure.
      - open() flags: O_EXCL, O_NONBLOCK, O_APPEND differ between newlib and
        BeOS.

    Known limits:
      - _fstat_r fills in only st_mode (character device if the descriptor is
        a terminal, regular file otherwise) and st_blksize. That is all
        newlib's stdio needs. struct stat differs between newlib and BeOS and
        is not translated yet, so fstat()/stat() give no size or times.
      - Only the stubs that stdio, malloc and process exit need are provided.
        _stat_r, _link_r, _gettimeofday_r, _times_r, ... are
        missing and will show up as undefined symbols when something uses them.
      - The image search assumes libroot's image ID is below 512 (it is loaded
        at boot, so it is a small number), and skips the kernel's own
        exports by address (see libroot_symbol()).
*/

#include <errno.h>
#include <fcntl.h>
#include <reent.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

/* libroot, by name (none of these exist in newlib) */
extern int get_image_symbol(int image, const char *name, int symbolType, void **location);
extern int *_errnop(void);
extern void *malloc(size_t);
extern void *calloc(size_t, size_t);
extern void *realloc(void *, size_t);
extern void free(void *);

/*
    The symbol type, error codes and open() flags below (BOS_SYMBOL_TYPE_ANY,
    the BOS_* error codes, BOS_O_*) are numeric values of the BeOS R5 public
    interface (kernel/image.h, support/Errors.h, posix/fcntl.h). They were
    checked against the headers shipped with BeOS R5 and are given our own
    names; no header text is copied. BOS_MAX_IMAGE_ID and
    BOS_USER_SPACE_START are our own choices, not from a header.
*/
#define BOS_SYMBOL_TYPE_ANY 5   /* B_SYMBOL_TYPE_ANY (DATA is 1, TEXT is 2) */
#define BOS_MAX_IMAGE_ID 512

/* BeOS error codes (Errors.h): LONG_MIN + n */
#define BOS_NO_MEMORY          ((int) 0x80000000)
#define BOS_IO_ERROR           ((int) 0x80000001)
#define BOS_PERMISSION_DENIED  ((int) 0x80000002)
#define BOS_BAD_VALUE          ((int) 0x80000005)
#define BOS_INTERRUPTED        ((int) 0x8000000a)
#define BOS_WOULD_BLOCK        ((int) 0x8000000b)
#define BOS_NOT_ALLOWED        ((int) 0x8000000f)
#define BOS_FILE_ERROR         ((int) 0x80006000)
#define BOS_FILE_EXISTS        ((int) 0x80006002)
#define BOS_ENTRY_NOT_FOUND    ((int) 0x80006003)
#define BOS_NOT_A_DIRECTORY    ((int) 0x80006005)
#define BOS_DEVICE_FULL        ((int) 0x80006007)
#define BOS_IS_A_DIRECTORY     ((int) 0x80006009)
#define BOS_NO_MORE_FDS        ((int) 0x8000600a)
#define BOS_BUSTED_PIPE        ((int) 0x8000600d)
#define BOS_POSIX_E2BIG        ((int) 0x80007001)
#define BOS_POSIX_ECHILD       ((int) 0x80007002)
#define BOS_POSIX_EFBIG        ((int) 0x80007004)
#define BOS_POSIX_ENOSYS       ((int) 0x80007009)
#define BOS_POSIX_ENOTTY       ((int) 0x8000700a)
#define BOS_POSIX_ESRCH        ((int) 0x8000700d)
#define BOS_POSIX_ERANGE       ((int) 0x80007011)

/* BeOS open() flags (fcntl.h); O_RDONLY/O_WRONLY/O_RDWR are 0/1/2 in both */
#define BOS_O_NONBLOCK 0x0080
#define BOS_O_EXCL     0x0100
#define BOS_O_CREAT    0x0200
#define BOS_O_TRUNC    0x0400
#define BOS_O_APPEND   0x0800

static const struct { int bos; int posix; } errno_map[] = {
    { BOS_NO_MEMORY,         ENOMEM  },
    { BOS_IO_ERROR,          EIO     },
    { BOS_PERMISSION_DENIED, EACCES  },
    { BOS_BAD_VALUE,         EINVAL  },
    { BOS_INTERRUPTED,       EINTR   },
    { BOS_WOULD_BLOCK,       EAGAIN  },
    { BOS_NOT_ALLOWED,       EPERM   },
    { BOS_FILE_ERROR,        EBADF   },
    { BOS_FILE_EXISTS,       EEXIST  },
    { BOS_ENTRY_NOT_FOUND,   ENOENT  },
    { BOS_NOT_A_DIRECTORY,   ENOTDIR },
    { BOS_DEVICE_FULL,       ENOSPC  },
    { BOS_IS_A_DIRECTORY,    EISDIR  },
    { BOS_NO_MORE_FDS,       EMFILE  },
    { BOS_BUSTED_PIPE,       EPIPE   },
    { BOS_POSIX_E2BIG,       E2BIG   },
    { BOS_POSIX_ECHILD,      ECHILD  },
    { BOS_POSIX_EFBIG,       EFBIG   },
    { BOS_POSIX_ENOSYS,      ENOSYS  },
    { BOS_POSIX_ENOTTY,      ENOTTY  },
    { BOS_POSIX_ESRCH,       ESRCH   },
    { BOS_POSIX_ERANGE,      ERANGE  },
};

static int translate_errno(int e)
{
    size_t i;
    for (i = 0; i < sizeof errno_map / sizeof errno_map[0]; i++)
        if (errno_map[i].bos == e)
            return errno_map[i].posix;
    return e > 0 && e < 0x8000 ? e : EIO;
}

/*
    Find a function exported by libroot.

    The kernel is itself an image (image ID 1) and exports its own write(),
    read(), close(), ... for kernel add-ons, so the first image that has the
    symbol may be the kernel. Its functions live in low kernel memory and
    calling one from user code faults ("access to kernel page"). User space
    starts at 0x80000000, so accept only results at or above that.
*/
#define BOS_USER_SPACE_START 0x80000000UL

static void *libroot_symbol(const char *name)
{
    void *p;
    int id;
    for (id = 1; id <= BOS_MAX_IMAGE_ID; id++)
        if (get_image_symbol(id, name, BOS_SYMBOL_TYPE_ANY, &p) == 0
            && (unsigned long) p >= BOS_USER_SPACE_START)
            return p;
    return 0;
}

/* Function pointers are transition vectors in this ABI, which is what
   get_image_symbol() returns for text symbols, so they can be called as is. */
#define LIBROOT_FN(var, name) \
    do { if (!(var)) (var) = libroot_symbol(name); } while (0)

static int fail(struct _reent *r, int err)
{
    r->_errno = err;
    return -1;
}

static int fail_from_libroot(struct _reent *r)
{
    return fail(r, translate_errno(*_errnop()));
}

_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t n)
{
    static int (*fn)(int, const void *, size_t);
    int rc;
    LIBROOT_FN(fn, "write");
    if (!fn)
        return fail(r, ENOSYS);
    rc = fn(fd, buf, n);
    return rc < 0 ? fail_from_libroot(r) : rc;
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t n)
{
    static int (*fn)(int, void *, size_t);
    int rc;
    LIBROOT_FN(fn, "read");
    if (!fn)
        return fail(r, ENOSYS);
    rc = fn(fd, buf, n);
    return rc < 0 ? fail_from_libroot(r) : rc;
}

int _close_r(struct _reent *r, int fd)
{
    static int (*fn)(int);
    LIBROOT_FN(fn, "close");
    if (!fn)
        return fail(r, ENOSYS);
    return fn(fd) < 0 ? fail_from_libroot(r) : 0;
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t offset, int whence)
{
    static long long (*fn)(int, long long, int);
    long long rc;
    LIBROOT_FN(fn, "lseek");
    if (!fn)
        return fail(r, ENOSYS);
    rc = fn(fd, (long long) offset, whence);
    if (rc < 0)
        return fail_from_libroot(r);
    if (rc > 0x7fffffffLL)
        return fail(r, EFBIG);
    return (_off_t) rc;
}

int _open_r(struct _reent *r, const char *path, int flags, int mode)
{
    static int (*fn)(const char *, int, int);
    int bflags = flags & 3;     /* access mode */
    int rc;
    LIBROOT_FN(fn, "open");
    if (!fn)
        return fail(r, ENOSYS);
    if (flags & O_CREAT)    bflags |= BOS_O_CREAT;
    if (flags & O_TRUNC)    bflags |= BOS_O_TRUNC;
    if (flags & O_EXCL)     bflags |= BOS_O_EXCL;
    if (flags & O_APPEND)   bflags |= BOS_O_APPEND;
    if (flags & O_NONBLOCK) bflags |= BOS_O_NONBLOCK;
    rc = fn(path, bflags, mode);
    return rc < 0 ? fail_from_libroot(r) : rc;
}

int _unlink_r(struct _reent *r, const char *path)
{
    static int (*fn)(const char *);
    LIBROOT_FN(fn, "unlink");
    if (!fn)
        return fail(r, ENOSYS);
    return fn(path) < 0 ? fail_from_libroot(r) : 0;
}

int _isatty_r(struct _reent *r, int fd)
{
    static int (*fn)(int);
    LIBROOT_FN(fn, "isatty");
    if (!fn) {
        r->_errno = ENOSYS;
        return 0;
    }
    return fn(fd) ? 1 : 0;
}

int _fstat_r(struct _reent *r, int fd, struct stat *st)
{
    memset(st, 0, sizeof *st);
    st->st_mode = _isatty_r(r, fd) ? (S_IFCHR | 0620) : (S_IFREG | 0644);
    st->st_blksize = 1024;
    return 0;
}

int _getpid_r(struct _reent *r)
{
    static int (*fn)(void);
    LIBROOT_FN(fn, "getpid");
    if (!fn) {
        r->_errno = ENOSYS;
        return 1;
    }
    return fn();
}

int _kill_r(struct _reent *r, int pid, int sig)
{
    static int (*fn)(int, int);
    LIBROOT_FN(fn, "kill");
    if (!fn)
        return fail(r, ENOSYS);
    return fn(pid, sig) < 0 ? fail_from_libroot(r) : 0;
}

void _exit(int status)
{
    static void (*fn)(int);
    LIBROOT_FN(fn, "_exit");
    if (fn && fn != (void (*)(int)) _exit)
        fn(status);
    for (;;)
        ;
}

/*
    newlib's getentropy() (used by libstdc++'s std::random_device) calls this.
    BeOS R5 has no entropy source: mix random() with the system clock. NOT
    cryptographically secure; fine for seeding simulations and tests only.
*/
extern long random(void);
extern long long system_time(void);     /* libroot: microseconds since boot */

int _getentropy_r(struct _reent *r, void *buf, size_t len)
{
    unsigned char *p = (unsigned char *) buf;
    size_t i;

    if (len > 256)
        return fail(r, EIO);
    for (i = 0; i < len; i++) {
        unsigned long long t = (unsigned long long) system_time();
        unsigned long v = (unsigned long) random();
        p[i] = (unsigned char) ((v ^ (v >> 8) ^ (v >> 16) ^ t ^ (t >> 8)) & 0xff);
    }
    return 0;
}

/* malloc is provided by libroot (newlib is built with MALLOC_PROVIDED). */
void *_malloc_r(struct _reent *r, size_t n)
{
    void *p = malloc(n);
    if (!p)
        r->_errno = ENOMEM;
    return p;
}

void *_calloc_r(struct _reent *r, size_t n, size_t size)
{
    void *p = calloc(n, size);
    if (!p)
        r->_errno = ENOMEM;
    return p;
}

void *_realloc_r(struct _reent *r, void *old, size_t n)
{
    void *p = realloc(old, n);
    if (!p && n)
        r->_errno = ENOMEM;
    return p;
}

void _free_r(struct _reent *r, void *p)
{
    (void) r;
    free(p);
}
