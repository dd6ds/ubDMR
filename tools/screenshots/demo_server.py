#!/usr/bin/env python3
# Minimal stand-in for a BrandMeister master (Rewind terminal port) used only
# to take the README screenshots: accepts any login and plays calls from
# fictional stations on talkgroup 262. No real callsigns or DMR IDs.
import os, socket, struct, sys, threading, time

OWN_ID = 1234567
# Fictional stations; their callsigns come from the seeded lookup cache.
GROUP_CALLS = [(2629901, 3.0), (2629902, 2.5), (2629903, 3.0)]
LONG_CALLER = 3100001          # talks in a loop so a call is always on air
PRIVATE_CALLER = 2629901
SIL72 = bytes.fromhex("B9E881526173002A6B")
# While this file exists the server stays quiet (shoot.sh: transmit shot).
QUIET = "/tmp/demo_quiet"


def rw(t, payload, seq=0, flags=0):
    return b"REWIND01" + struct.pack("<HHIH", t, flags, seq, len(payload)) + payload


EXP = [0] * 512
LOG = [0] * 256
x = 1
for i in range(255):
    EXP[i] = x
    LOG[x] = i
    x <<= 1
    if x & 0x100:
        x ^= 0x11D
for i in range(255, 512):
    EXP[i] = EXP[i - 255]


def gmul(a, b):
    return 0 if a == 0 or b == 0 else EXP[LOG[a] + LOG[b]]


def lc(flco, dst, src, mask):
    d = bytes([flco, 0, 0]) + dst.to_bytes(3, "big") + src.to_bytes(3, "big")
    p = [0, 0, 0]
    for m in d:
        f = m ^ p[2]
        p[2] = p[1] ^ gmul(14, f)
        p[1] = p[0] ^ gmul(56, f)
        p[0] = gmul(64, f)
    return d + bytes([p[2] ^ mask, p[1] ^ mask, p[0] ^ mask])


def call(s, a, src, dst, private, seconds):
    flco = 3 if private else 0
    for k in range(3):
        s.sendto(rw(0x0911, lc(flco, dst, src, 0x96), k, 1), a)
    for i in range(int(seconds / 0.06)):
        if os.path.exists(QUIET):
            break
        s.sendto(rw(0x0920, SIL72 * 3, i, 1), a)
        time.sleep(0.06)
    s.sendto(rw(0x0912, lc(flco, dst, src, 0x99), 0, 1), a)


def traffic(s, a):
    time.sleep(0.5)
    for src, secs in GROUP_CALLS:
        call(s, a, src, 262, False, secs)
        time.sleep(0.8)
    call(s, a, PRIVATE_CALLER, OWN_ID, True, 2.0)
    time.sleep(0.8)
    while True:
        while os.path.exists(QUIET):
            time.sleep(0.5)
        call(s, a, LONG_CALLER, 262, False, 40.0)
        time.sleep(1.0)


def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", 54006))
    nonce = os.urandom(4)
    authed, started = set(), False
    while True:
        d, a = s.recvfrom(2048)
        t, _, _, n = struct.unpack("<HHIH", d[8:18])
        p = d[18:18 + n]
        if t == 0x0000:
            s.sendto(rw(0x0000 if a in authed else 0x0002, b"" if a in authed else nonce), a)
        elif t == 0x0003:
            authed.add(a)
            s.sendto(rw(0x0000, b""), a)
        elif t == 0x0901:
            typ, num = struct.unpack("<II", p)
            if typ == 7 and num == 262 and not started:
                started = True
                threading.Thread(target=traffic, args=(s, a), daemon=True).start()
        elif t == 0x0a10:
            _, src, dst, opt, _ = struct.unpack("<IIIHH", p[:16])
            if opt == 0:
                s.sendto(rw(0x0a11, struct.pack("<IIIB", 0, dst, src, 8)), a)


if __name__ == "__main__":
    threading.Thread(target=main, daemon=True).start()
    time.sleep(float(sys.argv[1]) if len(sys.argv) > 1 else 300)
