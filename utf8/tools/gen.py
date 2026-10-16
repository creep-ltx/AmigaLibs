#!/usr/bin/env python3
"""gen.py - writes src/tables.c for utf8.library from Python's own
Unicode database (unicodedata) and codecs, so no files need fetching.

    python3 -I tools/gen.py > src/tables.c      (make tables)

The tables are small on purpose (the library goes on 68000 Amigas):
ranges searched by halving, starts as UWORD where the BMP is enough."""
import codecs
import sys
import unicodedata as u

out = []
def w(s=""):
    out.append(s)

def arr(ctype, name, vals, per=10, fmt="0x%04X"):
    w("const %s %s[%d] = {" % (ctype, name, len(vals)))
    for i in range(0, len(vals), per):
        w("    " + ", ".join(fmt % v for v in vals[i:i + per]) + ",")
    w("};")
    w()

w("/* tables.c - written by tools/gen.py from Python %s's Unicode %s:"
  % (sys.version.split()[0], u.unidata_version))
w(" * do not edit; change gen.py and run `make tables`. */")
w('#include "tables.h"')
w()
w("const char u8_unicode_version[] = \"%s\";" % u.unidata_version)
w()

# ---- width ------------------------------------------------------------------
# 0: joins the character before (marks, joiners); 2: two columns (East
# Asian Wide and Fullwidth, emoji). Controls are left to the code.
def width(c):
    ch = chr(c)
    cat = u.category(ch)
    if c == 0xAD:
        return 1                        # soft hyphen shows, as in wcwidth
    if cat in ("Mn", "Me", "Cf") or 0x1160 <= c <= 0x11FF or 0xD7B0 <= c <= 0xD7FF:
        return 0
    if u.east_asian_width(ch) in ("W", "F"):
        return 2
    return 1

def ranges(f, value, lo=0, hi=0x110000):
    r = []
    start = None
    for c in range(lo, hi):
        if f(c) == value:
            if start is None:
                start = c
        elif start is not None:
            r.append((start, c - 1))
            start = None
    if start is not None:
        r.append((start, hi - 1))
    return r

# utf8.c's PLAINWIDE windows: every character two columns, none a mark
# (CJK text without a table search). Keep this list the same as there.
PLAINWIDE = [(0x3041, 0x3097), (0x30A1, 0x30FB), (0x3400, 0x4DC0), (0x4E00, 0xA000),
             (0xAC00, 0xD7A4), (0xFF01, 0xFF61), (0x1F300, 0x1F321), (0x1F600, 0x1F650),
             (0x20000, 0x2FFFE), (0x30000, 0x3FFFE)]
for a, b in PLAINWIDE:
    for c in range(a, b):
        assert width(c) == 2 and u.category(chr(c))[0] != "M", hex(c)

# (0x300 and up: below it everything printable is one column)
zero = ranges(width, 0, 0x300)
wide = ranges(width, 2, 0x300)
for name, rr in (("u8_zero", zero), ("u8_wide", wide)):
    arr("ULONG", name + "_lo", [a for a, b in rr], 8, "0x%05X")
    arr("ULONG", name + "_hi", [b for a, b in rr], 8, "0x%05X")
w("const LONG u8_zero_n = %d, u8_wide_n = %d;" % (len(zero), len(wide)))
w()

# ---- classes ----------------------------------------------------------------
# L letter, D digit (Nd), N other number, P punctuation and symbols,
# Z space, M mark, C control/format/unassigned/private. The BMP as runs;
# above it a few hand ranges in utf8.c.
CLS = {"C": 0, "L": 1, "D": 2, "N": 3, "P": 4, "Z": 5, "M": 6}
def cls(c):
    cat = u.category(chr(c))
    if cat == "Nd":
        return CLS["D"]
    k = cat[0]
    if k == "S":
        k = "P"
    return CLS[k]

starts, kinds = [], []
prev = None
for c in range(0x10000):
    k = cls(c)
    if k != prev:
        starts.append(c)
        kinds.append(k)
        prev = k
arr("UWORD", "u8_class_at", starts, 10)
arr("UBYTE", "u8_class_kind", kinds, 20, "%d")
w("const LONG u8_class_n = %d;" % len(starts))
w()

# ---- case -------------------------------------------------------------------
# runs of (first, count, delta, step 1 or 2): one character to one
def caseruns(f):
    p = []
    for c in range(0x110000):
        ch = chr(c)
        t = f(ch)
        if len(t) == 1 and t != ch:
            p.append((c, ord(t) - c))
    r = []
    for c, d in p:
        if r:
            s, n, dd, st = r[-1]
            if dd == d and n == 1 and c - s in (1, 2):
                r[-1] = (s, 2, d, c - s)
                continue
            if dd == d and n > 1 and c == s + n * st:
                r[-1] = (s, n + 1, d, st)
                continue
        r.append((c, 1, d, 1))
    return r

for name, f in (("u8_upper", str.upper), ("u8_lower", str.lower)):
    r = caseruns(f)
    # the lookup takes the last run starting at or before a character, so
    # a run must end before the next one starts
    for i in range(len(r) - 1):
        s0, n, d, st = r[i]
        assert s0 + (n - 1) * st < r[i + 1][0], hex(s0)
    assert all(n < 0x8000 for s0, n, d, st in r)
    arr("ULONG", name + "_at", [s for s, n, d, st in r], 8, "0x%05X")
    arr("UWORD", name + "_len", [n | (0x8000 if st == 2 else 0) for s, n, d, st in r], 10)
    arr("LONG", name + "_delta", [d for s, n, d, st in r], 10, "%d")
    w("const LONG %s_n = %d;" % (name, len(r)))
    w()

# ---- composition ------------------------------------------------------------
# letter = base + one mark, for Latin, Greek, Cyrillic, a few signs (Ohm,
# Kelvin, Angstrom) and kana (what NFD file names from a Mac or a phone
# hold). Sorted by the letter; the mark as an index into u8_marks, 127 for
# a letter that is just another one (Greek with oxia = with tonos), +128
# when the pair is never put back together (Unicode's exclusions).
pairs = []
for c in list(range(0x500)) + list(range(0x1E00, 0x2200)) + list(range(0x3040, 0x3100)):
    d = u.decomposition(chr(c))
    if d and not d.startswith("<"):
        p = [int(x, 16) for x in d.split()]
        if len(p) == 1:
            pairs.append((c, p[0], None, True))
        elif len(p) == 2:
            back = u.normalize("NFC", chr(p[0]) + chr(p[1])) == chr(c)
            pairs.append((c, p[0], p[1], not back))
# utf8.c looks for these only in two windows (EXCLUDED there): keep it so
for c, b, m, x in pairs:
    if x:
        assert 0x340 <= c < 0x388 or 0x1F70 <= c < 0x2130, hex(c)
marks = sorted(set(m for c, b, m, x in pairs if m is not None))
assert len(marks) < 127
arr("UWORD", "u8_marks", marks, 10)
arr("UWORD", "u8_comp_ch", [c for c, b, m, x in pairs], 10)
arr("UWORD", "u8_comp_base", [b for c, b, m, x in pairs], 10)
arr("UBYTE", "u8_comp_mark", [(127 if m is None else marks.index(m)) | (128 if x else 0)
                              for c, b, m, x in pairs], 16, "%d")
w("const LONG u8_marks_n = %d, u8_comp_n = %d;" % (len(marks), len(pairs)))
w()

# ---- character sets -----------------------------------------------------------
# the upper half (0x80-0xFF) of each 8-bit set as code points; 0 where the
# set has nothing. Latin-1 needs no table. The order is U8CS_ in utf8.h.
SETS = [
    ("iso8859_2", "ISO-8859-2"), ("iso8859_15", "ISO-8859-15"),
    ("iso8859_5", "ISO-8859-5"), ("iso8859_7", "ISO-8859-7"),
    ("iso8859_9", "ISO-8859-9"), ("cp1250", "windows-1250"),
    ("cp1251", "windows-1251"), ("cp1252", "windows-1252"),
    ("cp437", "IBM437"), ("cp850", "IBM850"), ("cp866", "IBM866"),
    ("koi8_r", "KOI8-R"), ("mac_roman", "macintosh"),
]
vals = []
for codec, name in SETS:
    for b in range(0x80, 0x100):
        try:
            s = bytes([b]).decode(codec)
            vals.append(ord(s) if len(s) == 1 else 0)
        except UnicodeDecodeError:
            vals.append(0)
arr("UWORD", "u8_sets", vals, 8)
w("const LONG u8_sets_n = %d;" % len(SETS))

print("\n".join(out))
