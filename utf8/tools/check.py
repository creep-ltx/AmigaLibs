#!/usr/bin/env python3
"""check.py - holds utf8.c (build/pc/dump's output on stdin) against
Python's own Unicode database and codecs. Prints the differences and
how many; exits 1 if any.  make pccheck"""
import sys
import unicodedata as u

UC = dict(LETTER=1, DIGIT=2, NUMBER=4, PUNCT=8, SPACE=0x10, MARK=0x20, CONTROL=0x40,
          UPPER=0x100, LOWER=0x200, WIDE=0x400)
SETS = ["latin_1", "iso8859_2", "iso8859_15", "iso8859_5", "iso8859_7", "iso8859_9",
        "cp1250", "cp1251", "cp1252", "cp437", "cp850", "cp866", "koi8_r", "mac_roman",
        "ascii", None]

def width(c):
    ch = chr(c)
    if c < 0x20 or 0x7F <= c < 0xA0 or 0xD800 <= c < 0xE000:
        return -1
    if c == 0xAD:
        return 1
    cat = u.category(ch)
    if cat in ("Mn", "Me", "Cf") or 0x1160 <= c <= 0x11FF or 0xD7B0 <= c <= 0xD7FF:
        return 0
    return 2 if u.east_asian_width(ch) in ("W", "F") else 1

def one(s, c):
    return ord(s) if len(s) == 1 else c

def kind(c):
    if 0xD800 <= c < 0xE000:
        return 0
    if 9 <= c <= 13:
        return UC["SPACE"] | UC["CONTROL"]
    cat = u.category(chr(c))
    if c >= 0x10000:                    # the hand rules above the BMP
        if c >= 0xF0000: return UC["CONTROL"]
        if c >= 0xE0000: return UC["MARK"] if 0xE0100 <= c < 0xE01F0 else UC["CONTROL"]
        if 0x1F000 <= c < 0x1FC00: return UC["PUNCT"]
        if 0x1D7CE <= c < 0x1D800: return UC["DIGIT"]
        return UC["MARK"] if width(c) == 0 else UC["LETTER"]
    if cat == "Nd": return UC["DIGIT"]
    return {"L": UC["LETTER"], "N": UC["NUMBER"], "P": UC["PUNCT"], "S": UC["PUNCT"],
            "Z": UC["SPACE"], "M": UC["MARK"], "C": UC["CONTROL"]}[cat[0]]

def base(c):
    if c < 0xC0:
        return c
    d = u.normalize("NFD", chr(c))
    d = d.replace("\u00d8", "O").replace("\u00f8", "o")    # Ø: no accent to Unicode
    return ord(d[0]) if len(d) > 1 and all(u.category(x)[0] == "M" for x in d[1:]) else None

bad = 0
lines = 0
smp_class_off = 0
for line in sys.stdin:
    f = line.split()
    if f[0] == "N":
        c = int(f[1], 16)
        # the scripts the table covers (Latin, Greek, Cyrillic, signs,
        # kana) and Hangul, made by sum
        if not (c < 0x500 or 0x1E00 <= c < 0x2200 or 0x3040 <= c < 0x3100 or c >= 0xAC00):
            continue
        d = bytes.fromhex(f[2]).decode()
        k = bytes.fromhex(f[3]).decode() if len(f) > 3 else ""
        want = u.normalize("NFD", chr(c))
        if d != want:
            bad += 1
            if bad < 30: print("%04X NFD %r, Python %r" % (c, d, want))
        if k != u.normalize("NFC", want):
            bad += 1
            if bad < 30: print("%04X NFC %r, Python %r" % (c, k, u.normalize("NFC", want)))
        continue
    if f[0] == "S":
        cs, b, got = int(f[1]), int(f[2], 16), int(f[3], 16)
        name = SETS[cs]
        if name is None:                # UTF-8: one byte alone is Latin-1
            want = b
        else:
            try:
                s = bytes([b]).decode(name)
                want = ord(s) if len(s) == 1 else 0xFFFD
            except UnicodeDecodeError:
                want = 0xFFFD
        if got != want:
            bad += 1
            if bad < 30: print("set %d byte %02x: %x, Python %x" % (cs, b, got, want))
        continue
    lines += 1
    c, w, up, lo, bs, cl = (int(x, 16) if i != 1 else int(x) for i, x in enumerate(f))
    ch = chr(c)
    errs = []
    if c > 0x10FFFF: continue
    if w != width(c): errs.append("width %d, Python %d" % (w, width(c)))
    if 0xD800 <= c < 0xE000: 
        if errs: print("%04X %s" % (c, errs)); bad += 1
        continue
    if up != one(ch.upper(), c): errs.append("upper %x, Python %x" % (up, one(ch.upper(), c)))
    if lo != one(ch.lower(), c): errs.append("lower %x, Python %x" % (lo, one(ch.lower(), c)))
    b = base(c)
    if b is not None and bs != b and c < 0x2000 + 0x3100:
        # the table covers Latin, Greek, Cyrillic and kana; the rest may differ
        if c < 0x500 or 0x1E00 <= c < 0x2000 or 0x3040 <= c < 0x3100:
            errs.append("base %x, Python %x" % (bs, b))
    k = kind(c)
    if w == 2: k |= UC["WIDE"]
    if k & UC["LETTER"]:
        if one(ch.lower(), c) != c: k |= UC["UPPER"]
        if one(ch.upper(), c) != c: k |= UC["LOWER"]
    if cl != k: errs.append("class %x, want %x" % (cl, k))
    if errs:
        bad += 1
        if bad < 30: print("%04X %s" % (c, "; ".join(errs)))
print("%d code points, %d differences (Unicode %s)" % (lines, bad, u.unidata_version))
sys.exit(1 if bad else 0)
