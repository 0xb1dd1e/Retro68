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
    Bridge between code built against newlib's conventions (libstdc++,
    libsupc++ and anything compiled with the newlib headers) and BeOS R5's
    libroot, for programs that use libroot as their C library.

    Each function lives in this one object file inside libretrocrt.a, so the
    linker only pulls it in when something refers to one of these symbols.
    When a program is linked against newlib's own libc.a instead, libc.a's
    definitions are found first and this file is not used.

    Known limit: errno numbering. __errno() hands out libroot's per-thread
    errno, whose values are BeOS error codes (offsets from LONG_MIN), while
    code compiled with newlib's headers compares against newlib's numbers
    (EIO is 5, ERANGE is 34). The values returned by the functions below
    follow newlib; errors set by libroot calls are not translated.
*/

#include <stddef.h>

/* libroot (BeOS R5) */
extern int *_errnop(void);              /* per-thread errno */
extern char *strerror(int errnum);
extern long random(void);
extern long long system_time(void);     /* microseconds since boot */

/* newlib's errno numbering (what libstdc++ was compiled with). */
#define EIO_ 5
#define ERANGE_ 34

/* newlib's <errno.h> defines errno as (*__errno()). */
int *__errno(void)
{
    return _errnop();
}

/* XSI strerror_r, which libstdc++ uses for std::system_error messages. */
int __xpg_strerror_r(int errnum, char *buf, size_t buflen)
{
    const char *msg = strerror(errnum);
    size_t i = 0;

    if (buflen == 0)
        return ERANGE_;
    if (!msg)
        msg = "Unknown error";
    while (msg[i] && i + 1 < buflen) {
        buf[i] = msg[i];
        i++;
    }
    buf[i] = '\0';
    return msg[i] ? ERANGE_ : 0;
}

/*
    Entropy for std::random_device. BeOS R5 has no entropy source, so this is
    NOT cryptographically secure: it mixes random() with the system clock.
    Fine for seeding simulations and tests, not for keys.
*/
unsigned int arc4random(void)
{
    unsigned long long t = (unsigned long long) system_time();
    unsigned long r = (unsigned long) random();
    return (unsigned int) (r ^ (r << 16) ^ t ^ (t >> 32));
}

int getentropy(void *buf, size_t len)
{
    unsigned char *p = (unsigned char *) buf;
    size_t i;

    if (len > 256) {
        *_errnop() = EIO_;
        return -1;
    }
    for (i = 0; i < len; i++)
        p[i] = (unsigned char) (arc4random() >> 8);
    return 0;
}
