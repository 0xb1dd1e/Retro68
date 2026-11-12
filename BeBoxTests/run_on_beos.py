#!/usr/bin/env python3
#
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

"""Run one program on a BeOS R5 machine and compare its output with a file.

    run_on_beos.py --binary hello.pef --expected expected/hello.txt [options]

The program is copied to the machine over FTP, started over telnet in the
remote directory, and its standard output and standard error (merged) are
compared with the expected text. Only Python 3's standard library is used.

Both services come from BeOS R5's net_server (Preferences > Network > FTP /
Telnet). They work the same on a real BeBox and on an emulator whose ports
are forwarded to the host.

Exit status (ctest-compatible): 0 the output matched, 1 it did not or the
program failed, 77 the test was skipped (the machine cannot be reached or no
login was supplied; ctest shows "Skipped").

Settings (command line, or environment for the first two):
    BEBOX_HOST      address of the machine           (default 192.168.77.2)
    BEBOX_USER      login name
    BEBOX_PASSWORD  password (only from the environment, never a command line)
"""
import argparse
import ftplib
import os
import re
import select
import socket
import sys
import time

SKIP = 77

IAC, DONT, DO, WONT, WILL, SB, SE = 255, 254, 253, 252, 251, 250, 240
ECHO, SGA = 1, 3
MARK = "__retro68_test_%d__"
# "$ " / "# " or BeOS's coloured "/boot/home>" prompt
PROMPT = r"[$#>](\x1b\[[0-9;]*m)* ?$"


class Telnet:
    """Just enough telnet to run commands: refuses every option except
    echo and suppress-go-ahead."""

    def __init__(self, host, port, timeout):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.timeout = timeout
        self.buffer = b""
        self.pending = b""

    def _negotiate(self, data):
        out = bytearray()
        i = 0
        data = self.pending + data
        self.pending = b""
        while i < len(data):
            b = data[i]
            if b != IAC:
                out.append(b)
                i += 1
                continue
            if i + 1 >= len(data):
                self.pending = data[i:]
                break
            cmd = data[i + 1]
            if cmd == IAC:
                out.append(IAC)
                i += 2
            elif cmd in (DO, DONT, WILL, WONT):
                if i + 2 >= len(data):
                    self.pending = data[i:]
                    break
                opt = data[i + 2]
                if cmd == DO:
                    reply = WILL if opt == SGA else WONT
                elif cmd == WILL:
                    reply = DO if opt in (ECHO, SGA) else DONT
                else:
                    reply = None
                if reply is not None:
                    self.sock.sendall(bytes([IAC, reply, opt]))
                i += 3
            elif cmd == SB:
                end = data.find(bytes([IAC, SE]), i)
                if end < 0:
                    self.pending = data[i:]
                    break
                i = end + 2
            else:
                i += 2
        return bytes(out)

    def read_until(self, pattern, timeout=None):
        deadline = time.time() + (timeout or self.timeout)
        regex = re.compile(pattern)
        while True:
            text = self.buffer.decode("latin-1")
            m = regex.search(text)
            if m:
                consumed = text[:m.end()]
                self.buffer = self.buffer[len(consumed.encode("latin-1")):]
                return consumed
            left = deadline - time.time()
            if left <= 0:
                raise TimeoutError("waiting for %r; got %r" % (pattern, text[-200:]))
            ready, _, _ = select.select([self.sock], [], [], left)
            if ready:
                data = self.sock.recv(4096)
                if not data:
                    raise ConnectionError("connection closed")
                self.buffer += self._negotiate(data)

    def write(self, text):
        self.sock.sendall(text.encode("latin-1").replace(bytes([IAC]), bytes([IAC, IAC])))

    def close(self):
        self.sock.close()


def login(t, user, password):
    text = t.read_until(r"(ogin: ?$|assword: ?$|" + PROMPT + ")", timeout=20)
    if re.search(r"ogin: ?$", text):
        t.write(user + "\r\n")
        text = t.read_until(r"(assword: ?$|" + PROMPT + ")")
    if re.search(r"assword: ?$", text):
        t.write((password or "") + "\r\n")
        t.read_until(PROMPT)


def run_command(t, command, n):
    """Run one shell command; return (output, exit status).

    The terminal echoes the typed command line and may redraw it with
    backspaces, so the output is taken from between two marker lines that
    stand alone on their lines (the echoed copies never do)."""
    begin = "__retro68_begin_%d__" % n
    end = "__retro68_end_%d__" % n
    # "echo" before the end marker: a program that ends without a newline
    # must not share a line with it.
    t.write("echo %s; %s; r=$?; echo; echo %s $r\r\n" % (begin, command, end))
    text = t.read_until(r"%s (\d+)\s*\r?\n" % end)
    status = int(re.search(r"%s (\d+)" % end, text).group(1))
    t.read_until(PROMPT)
    lines = text.replace("\r", "").split("\n")
    try:
        first = lines.index(begin)
    except ValueError:
        return text, status
    body = []
    for l in lines[first + 1:]:
        if l.startswith(end):
            break
        body.append(l)
    return "\n".join(body), status


def ensure_dir(ftp, path):
    parts = [p for p in path.split("/") if p]
    cur = ""
    for p in parts:
        cur += "/" + p
        try:
            ftp.mkd(cur)
        except ftplib.error_perm:
            pass            # already exists


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--binary", required=True, help="PEF file to run")
    ap.add_argument("--expected", required=True, help="text file with the expected output")
    ap.add_argument("--name", help="name on the BeOS machine (default: the binary's file name without .pef)")
    ap.add_argument("--host", default=os.environ.get("BEBOX_HOST", "192.168.77.2"))
    ap.add_argument("--ftp-port", type=int, default=21)
    ap.add_argument("--telnet-port", type=int, default=23)
    ap.add_argument("--user", default=os.environ.get("BEBOX_USER"))
    ap.add_argument("--remote-dir", default="/boot/home/retro68-tests",
                    help="directory on the BeOS machine (created if missing)")
    ap.add_argument("--timeout", type=int, default=30, help="seconds to wait for the program")
    ap.add_argument("--show", action="store_true", help="always print the program's output")
    args = ap.parse_args()
    password = os.environ.get("BEBOX_PASSWORD")

    name = args.name or re.sub(r"\.pef$", "", os.path.basename(args.binary))
    expected = open(args.expected, newline="").read()

    if not args.user:
        print("SKIP: no login: set BEBOX_USER and BEBOX_PASSWORD")
        return SKIP

    # 1. copy the program over
    try:
        ftp = ftplib.FTP()
        ftp.connect(args.host, args.ftp_port, timeout=10)
        ftp.login(args.user, password or "")
        ftp.set_pasv(True)
        ensure_dir(ftp, args.remote_dir)
        ftp.cwd(args.remote_dir)
        with open(args.binary, "rb") as f:
            ftp.storbinary("STOR " + name, f)
        ftp.quit()
    except (OSError, ftplib.Error) as e:
        print("SKIP: cannot reach FTP on %s:%d (%s)" % (args.host, args.ftp_port, e))
        return SKIP

    # 2. run it
    try:
        t = Telnet(args.host, args.telnet_port, args.timeout)
        login(t, args.user, password)
        out, _ = run_command(t, "cd %s && chmod +x %s" % (args.remote_dir, name), 1)
        out, status = run_command(t, "cd %s && ./%s 2>&1" % (args.remote_dir, name), 2)
        t.close()
    except (OSError, TimeoutError, ConnectionError) as e:
        print("SKIP: cannot run on %s:%d (%s)" % (args.host, args.telnet_port, e))
        return SKIP

    # telnet ends lines with CR LF and the shell may add a trailing newline
    # after output that had none; compare without trailing whitespace.
    got = out.rstrip()
    want = expected.replace("\r", "").rstrip()
    ok = (got == want and status == 0)
    if args.show or not ok:
        print("--- output of %s (exit status %d) ---" % (name, status))
        print(out)
        print("--- expected ---")
        print(expected)
    print("%s: %s" % (name, "PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
