/* utf8.c - utf8.library's work: plain C, built into the library and on
 * the PC for the tests. No recursion, no floating point, nothing kept
 * between calls: every call can run on any task at the same time.
 *
 * One rule runs through it all: a byte that is not part of good UTF-8 is
 * the Latin-1 character with that number, as it would be on the Amiga.
 * Nothing is ever lost or refused because the text is 8-bit. */
#include "port.h"
#include "tables.h"

#define ZWJ  0x200D
#define BAD  0xFFFD                 /* the replacement character */

/* ---- reading and writing one character ------------------------------------ */

/* one character at s (e: the end): its length, or 0 when the bytes there
 * are not good UTF-8 (then *s is one Latin-1 character) */
static LONG dec(const UBYTE *s, const UBYTE *e, ULONG *cp)
{
    ULONG c = *s, u;
    LONG k, i;
    if (c < 0x80) {
        *cp = c;
        return 1;
    }
    if (c < 0xC2 || c > 0xF4) return 0;
    k = c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    if (e - s < k) return 0;
    u = c & (0x7F >> k);
    for (i = 1; i < k; i++) {
        if ((s[i] & 0xC0) != 0x80) return 0;
        u = (u << 6) | (s[i] & 0x3F);
    }
    /* not too long a way to write a smaller one, no surrogates, not past
     * the last code point */
    if ((k == 3 && u < 0x800) || (k == 4 && u < 0x10000) || u > 0x10FFFF ||
        (u >= 0xD800 && u < 0xE000))
        return 0;
    *cp = u;
    return k;
}

/* the next character, never failing: a bad byte is Latin-1 */
static LONG get(const UBYTE *s, const UBYTE *e, ULONG *cp)
{
    LONG k = dec(s, e, cp);
    if (k) return k;
    *cp = *s;
    return 1;
}

/* get() with the common characters (up to three bytes, no special
 * cases) decoded in place: the hot loops' way */
static inline __attribute__((always_inline)) LONG getq(const UBYTE *p, const UBYTE *e, ULONG *cp)
{
    ULONG b = *p, t;
    LONG k;
    if (b < 0x80) {
        *cp = b;
        return 1;
    }
    if (b >= 0xC2 && b < 0xE0 && p + 1 < e && (p[1] & 0xC0) == 0x80) {
        *cp = ((b & 0x1F) << 6) | (p[1] & 0x3F);
        return 2;
    }
    if (b >= 0xE1 && b <= 0xEF && b != 0xED && p + 2 < e && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        *cp = ((b & 0x0F) << 12) | ((ULONG)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        return 3;
    }
    k = get(p, e, &t);
    *cp = t;
    return k;
}

static LONG enc(ULONG c, UBYTE *b)
{
    if (c < 0x80) {
        b[0] = (UBYTE)c;
        return 1;
    }
    if (c < 0x800) {
        b[0] = (UBYTE)(0xC0 | (c >> 6));
        b[1] = (UBYTE)(0x80 | (c & 0x3F));
        return 2;
    }
    if (c < 0x10000) {
        if (c >= 0xD800 && c < 0xE000) return 0;
        b[0] = (UBYTE)(0xE0 | (c >> 12));
        b[1] = (UBYTE)(0x80 | ((c >> 6) & 0x3F));
        b[2] = (UBYTE)(0x80 | (c & 0x3F));
        return 3;
    }
    if (c > 0x10FFFF) return 0;
    b[0] = (UBYTE)(0xF0 | (c >> 18));
    b[1] = (UBYTE)(0x80 | ((c >> 12) & 0x3F));
    b[2] = (UBYTE)(0x80 | ((c >> 6) & 0x3F));
    b[3] = (UBYTE)(0x80 | (c & 0x3F));
    return 4;
}

static LONG slen(CONST_STRPTR s, LONG len)
{
    return len < 0 ? (LONG)strlen((const char *)s) : len;
}

LONG Utf8Decode(CONST_STRPTR s, LONG len, ULONG *cp)
{
    const UBYTE *b = (const UBYTE *)s;
    ULONG c;
    LONG k;
    if (!s) return 0;
    if (len < 0) len = *b ? 4 : 0;          /* a 0 ends it: dec stops at it */
    if (len == 0) return 0;
    if ((k = dec(b, b + len, &c)) == 0) {
        c = *b;
        k = -1;
    }
    if (cp) *cp = c;
    return k;
}

LONG Utf8Encode(ULONG cp, STRPTR buf)
{
    UBYTE t[4];
    LONG k = enc(cp, t), i;
    if (buf) for (i = 0; i < k; i++) buf[i] = (char)t[i];
    return k;
}

/* ---- what a text is ----------------------------------------------------------- */

LONG Utf8Valid(CONST_STRPTR s, LONG len)
{
    const UBYTE *b = (const UBYTE *)s, *p, *e;
    ULONG c;
    LONG k;
    if (!s) return -1;
    e = b + slen(s, len);
    for (p = b; p < e; p += k) {
        if (*p < 0x80) {
            k = 1;
            continue;
        }
        if (!(k = dec(p, e, &c))) return p - b;
    }
    return -1;
}

LONG Utf8Check(CONST_STRPTR s, LONG len)
{
    const UBYTE *b = (const UBYTE *)s, *e;
    if (!s) return U8_ASCII;
    len = slen(s, len);
    for (e = b + len; b < e && *b < 0x80; b++)
        ;
    if (b == e) return U8_ASCII;
    return Utf8Valid((CONST_STRPTR)b, e - b) < 0 ? U8_UTF8 : U8_LATIN1;
}

/* ---- widths and classes ----------------------------------------------------- */

/* is c in one of n ranges lo[i]..hi[i] (sorted)? */
static int inranges(ULONG c, const ULONG *lo, const ULONG *hi, LONG n)
{
    LONG a = 0, z = n - 1;
    while (a <= z) {
        LONG m = (a + z) >> 1;
        if (c < lo[m]) z = m - 1;
        else if (c > hi[m]) a = m + 1;
        else return 1;
    }
    return 0;
}

/* blocks where every character is two columns and none joins another
 * (tools/gen.py checks this list): CJK text without a table search */
static int plainwide(ULONG c)
{
    if (c < 0x3041) return 0;
    if (c < 0xA000)
        return (c < 0x3097) || (c >= 0x30A1 && c < 0x30FB) || (c >= 0x3400 && c < 0x4DC0) || c >= 0x4E00;
    if (c < 0x1F300) return (c >= 0xAC00 && c < 0xD7A4) || (c >= 0xFF01 && c < 0xFF61);
    return (c < 0x1F321) || (c >= 0x1F600 && c < 0x1F650) || (c >= 0x20000 && c < 0x2FFFE) ||
           (c >= 0x30000 && c < 0x3FFFE);
}

LONG Utf8CharWidth(ULONG c)
{
    if (c < 0x20 || (c >= 0x7F && c < 0xA0)) return -1;
    if (c < 0x300) return 1;
    if (c > 0x10FFFF || (c >= 0xD800 && c < 0xE000)) return -1;
    if (plainwide(c)) return 2;
    if (inranges(c, u8_zero_lo, u8_zero_hi, u8_zero_n)) return 0;
    if (c >= 0x1100 && inranges(c, u8_wide_lo, u8_wide_hi, u8_wide_n)) return 2;
    return 1;
}

/* the last of n sorted starts at or before c, or -1 */
static LONG lastat(ULONG c, const ULONG *at, LONG n)
{
    LONG a = 0, z = n - 1, r = -1;
    while (a <= z) {
        LONG m = (a + z) >> 1;
        if (at[m] <= c) {
            r = m;
            a = m + 1;
        } else
            z = m - 1;
    }
    return r;
}

static ULONG mapcase(ULONG c, const ULONG *at, const UWORD *ln, const LONG *delta, LONG n)
{
    LONG i = lastat(c, at, n);
    ULONG off, cnt, step;
    if (i < 0) return c;
    off = c - at[i];
    cnt = ln[i] & 0x7FFF;
    step = ln[i] & 0x8000 ? 2 : 1;
    if (step == 2) {
        if (off & 1) return c;
        off >>= 1;
    }
    return off < cnt ? (ULONG)((LONG)c + delta[i]) : c;
}

ULONG Utf8Upper(ULONG c)
{
    if (c < 0x80) return c >= 'a' && c <= 'z' ? c - 32 : c;
    if (c < 0x100) {                        /* Latin-1 the quick way */
        if (c >= 0xE0 && c != 0xF7 && c != 0xFF) return c - 32;
        if (c == 0xFF) return 0x178;
        if (c == 0xB5) return 0x39C;
        return c;
    }
    return mapcase(c, u8_upper_at, u8_upper_len, u8_upper_delta, u8_upper_n);
}

ULONG Utf8Lower(ULONG c)
{
    if (c < 0x80) return c >= 'A' && c <= 'Z' ? c + 32 : c;
    if (c < 0x100) return c >= 0xC0 && c <= 0xDE && c != 0xD7 ? c + 32 : c;
    return mapcase(c, u8_lower_at, u8_lower_len, u8_lower_delta, u8_lower_n);
}

/* case folded: ς and σ and Σ the same, ſ and s and S */
static ULONG fold(ULONG c)
{
    return Utf8Lower(Utf8Upper(c));
}

static const UWORD kindbits[] = {
    UC_CONTROL, UC_LETTER, UC_DIGIT, UC_NUMBER, UC_PUNCT, UC_SPACE, UC_MARK
};

/* the class alone, without case and width */
static ULONG kind(ULONG c)
{
    if (c > 0x10FFFF || (c >= 0xD800 && c < 0xE000)) return 0;
    if (c < 0x10000) {
        LONG a = 0, z = u8_class_n - 1, r = 0;
        if (c >= 9 && c <= 13) return UC_SPACE | UC_CONTROL;
        while (a <= z) {
            LONG m = (a + z) >> 1;
            if (u8_class_at[m] <= c) {
                r = m;
                a = m + 1;
            } else
                z = m - 1;
        }
        return kindbits[u8_class_kind[r]];
    }
    if (c >= 0xF0000) return UC_CONTROL;                    /* private use */
    if (c >= 0xE0000) return c >= 0xE0100 && c < 0xE01F0 ? UC_MARK : UC_CONTROL;
    if (c >= 0x1F000 && c < 0x1FC00) return UC_PUNCT;       /* emoji, cards... */
    if (c >= 0x1D7CE && c < 0x1D800) return UC_DIGIT;       /* mathematical digits */
    if (Utf8CharWidth(c) == 0) return UC_MARK;
    return UC_LETTER;                       /* the scripts and ideographs */
}

ULONG Utf8Class(ULONG c)
{
    ULONG k = kind(c);
    if (Utf8CharWidth(c) == 2) k |= UC_WIDE;
    if (k & UC_LETTER) {
        if (Utf8Lower(c) != c) k |= UC_UPPER;
        if (Utf8Upper(c) != c) k |= UC_LOWER;
    }
    return k;
}

/* ---- accents ------------------------------------------------------------------ */

/* where c is in the composition table (sorted by the letter), or -1 */
static LONG compat(ULONG c)
{
    LONG a = 0, z = u8_comp_n - 1;
    if (c < 0xC0 || c > 0x30FF) return -1;
    while (a <= z) {
        LONG m = (a + z) >> 1;
        if (c < u8_comp_ch[m]) z = m - 1;
        else if (c > u8_comp_ch[m]) a = m + 1;
        else return m;
    }
    return -1;
}

/* where the letters that are just another one, and the pairs Unicode
 * never puts together, are (tools/gen.py checks it) */
#define EXCLUDED(c) (((c) >= 0x340 && (c) < 0x388) || ((c) >= 0x1F70 && (c) < 0x2130))

/* Hangul syllables are made by sum, not by table */
#define SBASE 0xAC00
#define LBASE 0x1100
#define VBASE 0x1161
#define TBASE 0x11A7
#define NSYL  11172

/* base + mark as one character, or 0 */
static ULONG compose(ULONG base, ULONG mark)
{
    LONG i, m = -1;
    if (base >= LBASE && base < LBASE + 19 && mark >= VBASE && mark < VBASE + 21)
        return SBASE + ((base - LBASE) * 21 + (mark - VBASE)) * 28;
    if (base >= SBASE && base < SBASE + NSYL && mark > TBASE && mark < TBASE + 28 &&
        (base - SBASE) % 28 == 0)
        return base + (mark - TBASE);
    /* the marks that join are U+0300-0345 and the kana's two (gen.py checks) */
    if (mark < 0x300 || (mark > 0x345 && mark != 0x3099 && mark != 0x309A) || base > 0xFFFF)
        return 0;
    for (i = 0; i < u8_marks_n; i++)
        if (u8_marks[i] == mark) {
            m = i;
            break;
        }
    if (m < 0) return 0;
    {                                   /* halve u8_comp_byb to the letter's pairs */
        LONG a = 0, z = u8_comp_n - 1, j;
        while (a <= z) {
            LONG h = (a + z) >> 1;
            j = u8_comp_byb[h];
            if (u8_comp_base[j] < base || (u8_comp_base[j] == base && (u8_comp_mark[j] & 0x7F) < m))
                a = h + 1;
            else
                z = h - 1;
        }
        /* a: the first pair of (base, m) or after; an excluded pair has bit 7 */
        for (; a < u8_comp_n; a++) {
            j = u8_comp_byb[a];
            if (u8_comp_base[j] != base || (u8_comp_mark[j] & 0x7F) != m) break;
            if (u8_comp_mark[j] == m) return u8_comp_ch[j];
        }
    }
    return 0;
}

/* letters without an accent that Unicode does not take apart */
static const UWORD plain[][2] = {
    { 0x00D8, 'O' }, { 0x00F8, 'o' }, { 0x0110, 'D' }, { 0x0111, 'd' },
    { 0x0126, 'H' }, { 0x0127, 'h' }, { 0x0131, 'i' }, { 0x0141, 'L' },
    { 0x0142, 'l' }, { 0x0166, 'T' }, { 0x0167, 't' }, { 0x0180, 'b' },
    { 0x0197, 'I' }, { 0x01B5, 'Z' }, { 0x01B6, 'z' }, { 0x01E4, 'G' },
    { 0x01E5, 'g' }, { 0x0268, 'i' }, { 0x1D7B, 'I' }
};

ULONG Utf8NoAccent(ULONG c)
{
    LONG i, n = 0;
    if (c < 0xC0) return c;
    while ((i = compat(c)) >= 0 && n++ < 4)
        c = u8_comp_base[i];
    if (c > 0x1D7B) return c;           /* past plain[]'s last */
    for (i = 0; i < (LONG)(sizeof(plain) / sizeof(plain[0])); i++)
        if (plain[i][0] == c) return plain[i][1];
    return c;
}

/* ---- whole characters (what a reader counts as one) ------------------------- */

static int isri(ULONG c) { return c >= 0x1F1E6 && c <= 0x1F1FF; }

/* most text, the quick way: the length of a character at p that is one
 * column wide and on its own - printable ASCII, or two bytes below U+0300
 * (Latin, IPA) - when what follows cannot join it (all that joins is
 * U+0300 or above, starting with 0xCC or more); else 0 */
static inline __attribute__((always_inline)) LONG quick(const UBYTE *p, const UBYTE *e)
{
    UBYTE b = *p;
    if (b < 0x80) {
        if (b < 0x20 || b == 0x7F) return 0;
        return p + 1 == e || p[1] < 0xCC ? 1 : 0;
    }
    if (b < 0xC2 || b > 0xCB || p + 1 == e || (p[1] & 0xC0) != 0x80 || (b == 0xC2 && p[1] < 0xA0))
        return 0;
    return p + 2 == e || p[2] < 0xCC ? 2 : 0;
}

/* does c join the character before it? marks, joiners, variation
 * selectors, skin tones, tags, Hangul vowels and finals */
static int extends(ULONG c)
{
    if (c < 0x300) return 0;
    if (c == ZWJ || c == 0x200C || (c >= 0x1F3FB && c <= 0x1F3FF) ||
        (c >= 0xE0020 && c <= 0xE007F) || (c >= 0x1160 && c <= 0x11FF) ||
        (c >= 0xD7B0 && c <= 0xD7FF))
        return 1;
    if (plainwide(c)) return 0;
    return (kind(c) & UC_MARK) != 0;
}

/* the end of the whole character starting at p; *first: its first code
 * point */
static const UBYTE *cluster(const UBYTE *p, const UBYTE *e, ULONG *first)
{
    ULONG c, d;
    LONG k;
    p += getq(p, e, &c);
    if (first) *first = c;
    if (c == '\r' && p < e && *p == '\n') return p + 1;
    if (c < 0x20 || (c >= 0x7F && c < 0xA0)) return p;
    /* all that joins is U+0300 or above, which starts with 0xCC or more:
     * most text ends its characters here */
    if (p >= e || *p < 0xCC) return p;
    if (isri(c)) {
        k = getq(p, e, &d);
        if (isri(d)) p += k;                /* a flag: two of them */
    }
    while (p < e) {
        k = getq(p, e, &d);
        if (!extends(d)) break;
        p += k;
        if (d == ZWJ && p < e) {            /* joins the next emoji too */
            k = getq(p, e, &d);
            if (d >= 0x20 && !(d >= 0x7F && d < 0xA0)) p += k;
        }
    }
    return p;
}

LONG Utf8Next(CONST_STRPTR s, LONG len, LONG pos)
{
    const UBYTE *b = (const UBYTE *)s;
    if (!s) return 0;
    len = slen(s, len);
    if (pos < 0) pos = 0;
    if (pos >= len) return len;
    return cluster(b + pos, b + len, NULL) - b;
}

/* the start of the code point that ends at q (b: the text's start) */
static const UBYTE *back1(const UBYTE *b, const UBYTE *q, const UBYTE *e)
{
    const UBYTE *p = q - 1;
    ULONG c;
    int i;
    for (i = 0; i < 3 && p > b && (*p & 0xC0) == 0x80; i++)
        p--;
    if (p < q - 1 && dec(p, e, &c) == q - p) return p;
    return q - 1;
}

LONG Utf8Prev(CONST_STRPTR s, LONG len, LONG pos)
{
    const UBYTE *b = (const UBYTE *)s, *e, *p, *q, *n;
    ULONG c, d;
    if (!s) return 0;
    len = slen(s, len);
    if (pos > len) pos = len;
    if (pos <= 0) return 0;
    e = b + len;
    /* back to a code point that surely starts a character: not one that
     * joins, not after a joiner, and before a run of flags */
    p = back1(b, b + pos, e);
    while (p > b) {
        get(p, e, &c);
        q = back1(b, p, e);
        get(q, e, &d);
        if (!extends(c) && d != ZWJ && !isri(c) && !(c == '\n' && d == '\r')) break;
        p = q;
    }
    /* then forward, character by character, to the one before pos */
    for (;;) {
        n = cluster(p, e, NULL);
        if (n >= b + pos) return p - b;
        p = n;
    }
}

LONG Utf8Length(CONST_STRPTR s, LONG len)
{
    const UBYTE *p = (const UBYTE *)s, *e;
    LONG n = 0;
    if (!s) return 0;
    e = p + slen(s, len);
    while (p < e) {
        LONG k = quick(p, e);
        p = k ? p + k : cluster(p, e, NULL);
        n++;
    }
    return n;
}

/* the columns of the character starting with c */
static LONG cwidth(ULONG c)
{
    LONG w = Utf8CharWidth(c);
    if (w < 0) return 0;
    if (isri(c)) return 2;
    return w;
}

LONG Utf8Fit(CONST_STRPTR s, LONG len, LONG cols, LONG *width)
{
    const UBYTE *b = (const UBYTE *)s, *p, *e, *n;
    LONG w = 0, cw;
    ULONG c;
    if (!s) {
        if (width) *width = 0;
        return 0;
    }
    e = b + slen(s, len);
    p = b;
    while (p < e) {
        LONG k = quick(p, e);
        if (k) {
            n = p + k;
            cw = 1;
        } else {
            n = cluster(p, e, &c);
            cw = cwidth(c);
        }
        if (w + cw > cols) break;
        w += cw;
        p = n;
    }
    if (width) *width = w;
    return p - b;
}

LONG Utf8Width(CONST_STRPTR s, LONG len)
{
    LONG w;
    Utf8Fit(s, len, 0x7FFFFFFF, &w);
    return w;
}

/* ---- comparing and searching -------------------------------------------------- */

/* reads text as the keys it is compared by: accents put on their
 * letters, then without accents or case if asked */
typedef struct {
    const UBYTE *p, *e;
    ULONG flags;
    ULONG pend;                     /* a mark still to give, or 0 */
} Keys;

/* can what starts at p join the character before (compose() could put it
 * on)? Only U+0300-036F (bytes CC, CD), the kana's marks U+3099-309A
 * and Hangul's vowels and finals U+1161-11C2 (E1 85-87): the rest of
 * Cyrillic, Greek, CJK text is not decoded twice */
static inline __attribute__((always_inline)) int joins(const UBYTE *p, const UBYTE *e)
{
    UBYTE b = *p;
    if (b < 0xCC) return 0;
    if (b <= 0xCD) return 1;
    if (b == 0xE1) return p + 1 < e && p[1] >= 0x85 && p[1] <= 0x87;
    if (b == 0xE3) return p + 2 < e && p[1] == 0x82 && (p[2] == 0x99 || p[2] == 0x9A);
    return 0;
}

static void keysinit(Keys *k, CONST_STRPTR s, LONG len, ULONG flags)
{
    k->p = (const UBYTE *)s;
    k->e = k->p + slen(s, len);
    k->flags = flags;
    k->pend = 0;
}

static int nextkey(Keys *k, ULONG *key)
{
    ULONG c, m, t;
    LONG n, i;
    if (k->pend) {
        *key = k->pend;
        k->pend = 0;
        return 1;
    }
    if (k->p >= k->e) return 0;
    k->p += getq(k->p, k->e, &c);
    while (k->p < k->e && joins(k->p, k->e)) {
        n = get(k->p, k->e, &m);
        if (m < 0x300) break;
        if (EXCLUDED(m) && (i = compat(m)) >= 0 && (u8_comp_mark[i] & 0x80)) {
            ULONG m2 = (u8_comp_mark[i] & 0x7F) == 127 ? 0 : u8_marks[u8_comp_mark[i] & 0x7F];
            m = u8_comp_base[i];            /* U+0341 is U+0301; U+0344 is two marks */
            if (m2) {
                if (!(t = compose(c, m))) break;
                c = t;
                k->p += n;
                if ((t = compose(c, m2)) != 0) {
                    c = t;
                    continue;
                }
                if (!(k->flags & U8F_NOACCENT)) k->pend = m2;
                break;
            }
        }
        if (!(t = compose(c, m))) break;
        c = t;
        k->p += n;
    }
    /* as NFC has it: a letter that is just another one (U+0387 is ·) is
     * that one; a pair Unicode never puts together stays apart */
    while (EXCLUDED(c) && (i = compat(c)) >= 0 && (u8_comp_mark[i] & 0x80)) {
        m = u8_comp_mark[i] & 0x7F;
        c = u8_comp_base[i];
        if (m != 127) {
            if (!(k->flags & U8F_NOACCENT)) k->pend = u8_marks[m];
            break;
        }
    }
    if (k->flags & U8F_NOACCENT) {
        c = Utf8NoAccent(c);
        while (k->p < k->e && *k->p >= 0xCC) {  /* and the marks left over */
            n = get(k->p, k->e, &m);
            if (m < 0x300 || !(kind(m) & UC_MARK)) break;
            k->p += n;
        }
    }
    if (k->flags & U8F_NOCASE) c = fold(c);
    *key = c;
    return 1;
}

LONG Utf8Compare(CONST_STRPTR a, LONG alen, CONST_STRPTR b, LONG blen, ULONG flags)
{
    Keys x, y;
    ULONG p, q;
    int hp, hq;
    if (!a) a = "";
    if (!b) b = "";
    keysinit(&x, a, alen, flags);
    keysinit(&y, b, blen, flags);
    for (;;) {
        hp = nextkey(&x, &p);
        hq = nextkey(&y, &q);
        if (!hp || !hq) return hp - hq;
        if (p != q) return p < q ? -1 : 1;
    }
}

/* past the printable ASCII that is neither f1 nor f2 (each of them a
 * character of its own): where a search can skip quickly */
static const UBYTE *skipascii(const UBYTE *p, const UBYTE *e, ULONG f1, ULONG f2)
{
    while (p < e) {
        ULONG c = *p;
        if (c < 0x20 || c >= 0x7F || c == f1 || c == f2) break;
        if (p + 1 < e && p[1] >= 0xCC) break;
        p++;
    }
    return p;
}

LONG Utf8Find(CONST_STRPTR s, LONG len, CONST_STRPTR pat, LONG plen, ULONG flags, LONG *matchlen)
{
    const UBYTE *b = (const UBYTE *)s, *e, *p;
    Keys x, y;
    ULONG kx, ky, first, f1, f2;
    LONG k;
    if (matchlen) *matchlen = 0;
    if (!s || !pat) return -1;
    e = b + slen(s, len);
    plen = slen(pat, plen);
    keysinit(&y, pat, plen, flags);
    if (!nextkey(&y, &first)) return 0;     /* nothing to find: here */
    /* ASCII bytes that could start a match (ASCII never turns into
     * anything else without case or accent) */
    f1 = f2 = first < 0x80 ? first : 0x100;
    if ((flags & U8F_NOCASE) && f1 >= 'a' && f1 <= 'z') f2 = f1 - 32;
    for (p = b; p < e; p = k ? p + k : cluster(p, e, NULL)) {  /* at each whole character */
        if ((p = skipascii(p, e, f1, f2)) >= e) break;
        /* the first letter the quick way */
        if ((k = quick(p, e)) != 0) {
            kx = k == 1 ? *p : ((ULONG)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
            if (flags & U8F_NOACCENT) kx = Utf8NoAccent(kx);
            if (flags & U8F_NOCASE) kx = kx < 0x80 ? (kx >= 'A' && kx <= 'Z' ? kx + 32 : kx) : fold(kx);
            if (kx != first) continue;
        }
        keysinit(&x, (CONST_STRPTR)p, e - p, flags);
        if (!k) {                           /* the first letter, the long way */
            nextkey(&x, &kx);
            if (kx != first) continue;
            keysinit(&x, (CONST_STRPTR)p, e - p, flags);
        }
        keysinit(&y, pat, plen, flags);
        for (;;) {
            if (!nextkey(&y, &ky)) {
                /* the whole pattern: and not ending inside a character */
                const UBYTE *q = p;
                if (x.pend) break;          /* half of a letter */
                while (q < x.p) q = cluster(q, e, NULL);
                if (q != x.p) break;
                if (matchlen) *matchlen = x.p - p;
                return p - b;
            }
            if (!nextkey(&x, &kx) || kx != ky) break;
        }
    }
    return -1;
}

/* ---- writing text out ------------------------------------------------------- */

/* the output: up to size - 1 bytes and a 0; n counts what it all needs.
 * A character that does not fit stops the writing, so the output never
 * ends in half a character. */
typedef struct {
    UBYTE *buf;
    LONG size, n, w, full;          /* n: needed, w: written */
} Out;

static void outinit(Out *o, STRPTR buf, LONG size)
{
    o->buf = buf && size > 0 ? (UBYTE *)buf : NULL;     /* NULL: only measure */
    o->size = o->buf ? size : 0;
    o->n = o->w = o->full = 0;
}

static void put(Out *o, const UBYTE *t, LONG k)
{
    LONG i;
    if (!o->full && o->w + k <= o->size - 1) {
        for (i = 0; i < k; i++) o->buf[o->w + i] = t[i];
        o->w += k;
    } else
        o->full = 1;
    o->n += k;
}

static void putc8(Out *o, ULONG c)
{
    UBYTE t[4];
    LONG k = enc(c, t);
    if (!k) k = enc(BAD, t);
    put(o, t, k);
}

static LONG outend(Out *o)
{
    if (o->size > 0) o->buf[o->w] = 0;
    return o->n;
}

LONG Utf8Case(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, ULONG flags)
{
    const UBYTE *p = (const UBYTE *)s, *e;
    ULONG c;
    Out o;
    outinit(&o, buf, size);
    if (s) {
        e = p + slen(s, len);
        while (p < e) {
            p += get(p, e, &c);
            if (flags & U8F_UPPER) c = Utf8Upper(c);
            else if (flags & U8F_LOWER) c = Utf8Lower(c);
            putc8(&o, c);
        }
    }
    return outend(&o);
}

/* c taken apart into o: base first, then its marks in order */
static void decompose(Out *o, ULONG c)
{
    ULONG st[6];
    LONG n = 0, i;
    if (c >= SBASE && c < SBASE + NSYL) {
        ULONG si = c - SBASE, t = si % 28;
        putc8(o, LBASE + si / 588);
        putc8(o, VBASE + (si % 588) / 28);
        if (t) putc8(o, TBASE + t);
        return;
    }
    while ((i = compat(c)) >= 0 && n < 6) {
        UBYTE m = u8_comp_mark[i] & 0x7F;
        if (m != 127) st[n++] = u8_marks[m];  /* 127: just another letter */
        c = u8_comp_base[i];
    }
    putc8(o, c);
    while (n) putc8(o, st[--n]);
}

LONG Utf8Normalize(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, ULONG flags)
{
    Out o;
    Keys k;
    ULONG c;
    outinit(&o, buf, size);
    if (!s) return outend(&o);
    keysinit(&k, s, len, 0);
    if (flags & U8F_COMPOSE) {
        while (nextkey(&k, &c)) putc8(&o, c);
    } else
        while (k.p < k.e) {
            k.p += get(k.p, k.e, &c);
            if (flags & U8F_DECOMPOSE) decompose(&o, c);
            else putc8(&o, c);
        }
    return outend(&o);
}

/* ---- character sets ----------------------------------------------------------- */

/* the character of byte b (0x80 and up) in set cs, or 0 */
static ULONG setchar(LONG cs, UBYTE b)
{
    if (cs == U8CS_LATIN1) return b;
    if (cs == U8CS_ASCII) return 0;
    return u8_sets[((cs - 1) << 7) + (b - 0x80)];
}

/* the byte for c in set cs, or 0 */
static UBYTE setbyte(LONG cs, ULONG c)
{
    const UWORD *t;
    LONG i;
    if (c < 0x80) return (UBYTE)c;
    if (cs == U8CS_LATIN1) return c < 0x100 ? (UBYTE)c : 0;
    if (cs == U8CS_ASCII || c > 0xFFFF) return 0;
    t = u8_sets + ((cs - 1) << 7);
    for (i = 0; i < 128; i++)
        if (t[i] == c) return (UBYTE)(0x80 + i);
    return 0;
}

/* what to show for a character a set lacks, after its accent is gone:
 * Latin-1 text (itself turned into the set, '?' where that fails).
 * Sorted by the character: it is searched by halving (unit.c checks) */
static const struct { UWORD c; char s[4]; } near[] = {
    { 0x00A0, " " }, { 0x00A9, "(C)" }, { 0x00AB, "<<" }, { 0x00AE, "(R)" },
    { 0x00B7, "." }, { 0x00BB, ">>" }, { 0x00C6, "AE" }, { 0x00D7, "x" },
    { 0x00DF, "ss" }, { 0x00E6, "ae" }, { 0x0152, "OE" }, { 0x0153, "oe" },
    { 0x02BC, "'" }, { 0x02C6, "^" }, { 0x02DC, "~" },
    { 0x2002, " " }, { 0x2003, " " }, { 0x2009, " " }, { 0x200A, " " },
    { 0x2010, "-" }, { 0x2011, "-" }, { 0x2012, "-" }, { 0x2013, "-" },
    { 0x2014, "-" }, { 0x2015, "-" }, { 0x2018, "'" }, { 0x2019, "'" },
    { 0x201A, "," }, { 0x201B, "'" }, { 0x201C, "\"" }, { 0x201D, "\"" },
    { 0x201E, "\"" }, { 0x201F, "\"" }, { 0x2020, "+" }, { 0x2022, "\xB7" },
    { 0x2026, "..." }, { 0x2030, "%" }, { 0x2032, "'" }, { 0x2033, "\"" },
    { 0x2039, "<" }, { 0x203A, ">" }, { 0x2044, "/" }, { 0x20AC, "EUR" },
    { 0x2122, "TM" }, { 0x2190, "<-" }, { 0x2191, "^" }, { 0x2192, "->" },
    { 0x2193, "v" }, { 0x2194, "<->" }, { 0x21D0, "<=" }, { 0x21D2, "=>" },
    { 0x2212, "-" }, { 0x2215, "/" }, { 0x2217, "*" }, { 0x221E, "oo" },
    { 0x2248, "~" }, { 0x2260, "!=" }, { 0x2264, "<=" }, { 0x2265, ">=" },
    { 0x2302, "^" }, { 0x2318, "#" }, { 0x2588, "#" }, { 0x2591, ":" },
    { 0x2592, "%" }, { 0x2593, "#" }, { 0x25A0, "#" }, { 0x25B2, "^" },
    { 0x25B6, ">" }, { 0x25BA, ">" }, { 0x25BC, "v" }, { 0x25C0, "<" },
    { 0x25C4, "<" }, { 0x25CB, "o" }, { 0x25CF, "*" }, { 0x2605, "*" },
    { 0x2606, "*" }, { 0x2713, "v" }, { 0x2714, "v" }, { 0x2717, "x" },
    { 0x2718, "x" }, { 0x3000, "  " }, { 0xFB01, "fi" }, { 0xFB02, "fl" }
};

/* box drawing (U+2500-257F) as - | + (and / \\ X for the diagonals) */
static const char boxtab[] =
    "--||--||--||++++++++++++++++++++++++++++++++++++++++++++++++++++"
    "++++++++++++--||-|+++++++++++++++++++++++++++++++/\\X-|-|-|-|-|-|";

static char boxchar(ULONG c)
{
    return boxtab[c - 0x2500];
}

/* U+2010-2026, the punctuation most text has, as near[] has it (one
 * byte; '.' for the ellipsis, written as three) */
static const char punct[23] = {
    '-', '-', '-', '-', '-', '-', '?', '?', '\'', '\'', ',', '\'', '"', '"', '"', '"',
    '+', '?', (char)0xB7, '?', '?', '?', '.'
};

/* ---- other scripts in Latin letters ------------------------------------------- */

/* what a set that lacks them shows for Cyrillic, Greek, kana and Hangul:
 * the letters spelt out (Привет Privet, Ωμέγα Omega, こんにちは
 * konnichiwa, 한국 hanguk), as signs and names are written for those who
 * cannot read the script. Chinese characters and the kanji in Japanese
 * have no spelling short of a dictionary: they stay '?'. */

/* Cyrillic U+0400-045F (Russian, plus Ukrainian, Belarusian, Serbian and
 * Macedonian letters); the capitals are the small ones spelt with a
 * capital first letter */
static const char cyr[96][5] = {
    "e", "yo", "dj", "gj", "ye", "dz", "i", "yi", "j", "lj", "nj", "c", "kj", "i", "u", "dz",
    "a", "b", "v", "g", "d", "e", "zh", "z", "i", "y", "k", "l", "m", "n", "o", "p",
    "r", "s", "t", "u", "f", "kh", "ts", "ch", "sh", "shch", "\"", "y", "'", "e", "yu", "ya",
    "a", "b", "v", "g", "d", "e", "zh", "z", "i", "y", "k", "l", "m", "n", "o", "p",
    "r", "s", "t", "u", "f", "kh", "ts", "ch", "sh", "shch", "\"", "y", "'", "e", "yu", "ya",
    "e", "yo", "dj", "gj", "ye", "dz", "i", "yi", "j", "lj", "nj", "c", "kj", "i", "u", "dz",
};

/* Greek U+03B1-03C9, the capitals 0x20 lower; U+03C2 (final sigma) is
 * where no capital is */
static const char grk[25][3] = {
    "a", "v", "g", "d", "e", "z", "i", "th", "i", "k", "l", "m", "n", "x", "o", "p",
    "r", "s", "s", "t", "y", "f", "ch", "ps", "o",
};

/* kana U+3041-3096 (hiragana; katakana are the same 0x60 higher), in
 * Hepburn. '*' marks the small ones that change the kana before; '-' the
 * small tsu, which doubles the next consonant */
static const char kana[86][4] = {
    "*a", "a", "*i", "i", "*u", "u", "*e", "e", "*o", "o",
    "ka", "ga", "ki", "gi", "ku", "gu", "ke", "ge", "ko", "go",
    "sa", "za", "shi", "ji", "su", "zu", "se", "ze", "so", "zo",
    "ta", "da", "chi", "ji", "-", "tsu", "zu", "te", "de", "to", "do",
    "na", "ni", "nu", "ne", "no",
    "ha", "ba", "pa", "hi", "bi", "pi", "fu", "bu", "pu", "he", "be", "pe", "ho", "bo", "po",
    "ma", "mi", "mu", "me", "mo",
    "*ya", "ya", "*yu", "yu", "*yo", "yo",
    "ra", "ri", "ru", "re", "ro", "*wa", "wa", "i", "e", "o", "n", "vu", "ka", "ke",
};

/* Hangul syllables (Revised Romanization, letter by letter): the first
 * consonant, the vowel, the last consonant as it sounds */
static const char hl[19][3] = {
    "g", "kk", "n", "d", "tt", "r", "m", "b", "pp", "s", "ss", "", "j", "jj", "ch", "k", "t", "p", "h"
};
static const char hv[21][4] = {
    "a", "ae", "ya", "yae", "eo", "e", "yeo", "ye", "o", "wa", "wae", "oe", "yo", "u", "wo", "we",
    "wi", "yu", "eu", "ui", "i"
};
static const char ht[28][3] = {
    "", "k", "k", "k", "n", "n", "n", "t", "l", "k", "m", "l", "l", "l", "p", "l",
    "m", "p", "p", "t", "t", "ng", "t", "t", "k", "t", "p", "t"
};

/* what the kana before left, so the next can join it: valid while
 * nothing else has been written since (at == the output's count) */
typedef struct {
    LONG at, w;                 /* w: where the kana before starts in the output */
    char last[5];               /* its spelling, as written */
    UBYTE tsu;                  /* a small tsu waits for its consonant */
} Tr;

static int vowel(char c)
{
    return c == 'a' || c == 'i' || c == 'u' || c == 'e' || c == 'o';
}

static void puts_(Out *o, const char *s)
{
    LONG k = 0;
    while (s[k]) k++;
    put(o, (const UBYTE *)s, k);
}

/* take back the last byte written (the kana before's vowel) */
static void unput(Out *o)
{
    if (!o->full) o->w--;
    o->n--;
}

/* a spelling with its first letter a capital, in one piece (a character
 * is written whole or not at all) */
static void putcap(Out *o, const char *s, int cap)
{
    UBYTE t[8];
    LONG k;
    for (k = 0; s[k] && k < 7; k++) t[k] = (UBYTE)s[k];
    if (k && cap && t[0] >= 'a' && t[0] <= 'z') t[0] -= 32;
    put(o, t, k);
}

/* 1: it rewrote the end of the kana before */
static int kanaout1(Out *o, Tr *tr, ULONG c)
{
    char *l = tr->last;
    const char *r;
    LONG n;
    int joined = tr->at == o->n;
    if (!joined) {
        l[0] = 0;
        tr->tsu = 0;
    }
    for (n = 0; l[n]; n++)
        ;
    if (c == 0x30FC) {                      /* the long vowel mark: the vowel again */
        if (n && vowel(l[n - 1])) {
            UBYTE t = (UBYTE)l[n - 1];
            put(o, &t, 1);
        } else
            puts_(o, "-");
        tr->at = o->n;
        return 0;
    }
    if (c == 0x309D || c == 0x309E || c == 0x30FD || c == 0x30FE) {     /* the kana again */
        puts_(o, l);
        tr->at = o->n;
        return 0;
    }
    if (c >= 0x30F7) {                      /* va vi ve vo */
        static const char v4[4][3] = { "va", "vi", "ve", "vo" };
        r = v4[c - 0x30F7];
    } else
        r = kana[(c >= 0x30A1 ? c - 0x60 : c) - 0x3041];
    if (r[0] == '-') {                      /* small tsu */
        tr->tsu = 1;
        tr->at = o->n;
        return 0;
    }
    if (r[0] == '*') {                      /* small: joins the kana before */
        int rw = 0;
        r++;
        if (n >= 2 && l[n - 1] == 'i' && r[0] == 'y') {
            /* ki + ya = kya; shi, chi, ji + ya = sha, cha, ja */
            unput(o);
            if ((n == 3 && l[1] == 'h') || (n == 2 && l[0] == 'j')) r++;
            l[n - 1] = 0;
            rw = 1;
        } else if (n >= 2 && vowel(l[n - 1])) {
            /* fu + a = fa, te + i = ti, shi + e = she, ku + wa = kwa */
            unput(o);
            l[n - 1] = 0;
            rw = 1;
        } else if (n == 1 && l[0] == 'u' && vowel(r[0])) {
            unput(o);                       /* u + i = wi */
            puts_(o, "w");
            l[0] = 'w';
            rw = 1;
        }
        for (n = 0; l[n]; n++)
            ;
        puts_(o, r);
        while (*r && n < 4) l[n++] = *r++;
        l[n] = 0;
        tr->tsu = 0;
        tr->at = o->n;
        return rw;
    }
    if (tr->tsu && !vowel(r[0]) && r[0] != 'n') {
        UBYTE t = r[0] == 'c' ? 't' : (UBYTE)r[0];     /* tchi, kka, ssha */
        put(o, &t, 1);
    } else if (n == 1 && l[0] == 'n' && (vowel(r[0]) || r[0] == 'y')) puts_(o, "'");
    tr->tsu = 0;
    puts_(o, r);
    for (n = 0; r[n] && n < 4; n++) l[n] = r[n];
    l[n] = 0;
    tr->at = o->n;
    return 0;
}

/* a small kana rewrites the end of the one before (ki + ya = kya): if the
 * result does not fit, both go, so a cut result is always the start of
 * the whole one */
static void kanaout(Out *o, Tr *tr, ULONG c)
{
    LONG w0 = o->w, f0 = o->full;
    int rewrote = kanaout1(o, tr, c);
    if (!f0 && o->full) o->w = rewrote ? tr->w : w0;
    else if (!rewrote) tr->w = w0;          /* where this kana's spelling starts */
}

/* Hangul syllable s (from U+AC00) spelt into t: its length. The
 * divisions by 28 and 21 as multiplications (exact for every syllable:
 * tests/unit.c checks): a 68000 has no 32-bit divide */
static LONG hangul(UWORD s, UBYTE *t)
{
    UWORD q = (UWORD)(((ULONG)s * (UWORD)9363) >> 18);     /* s / 28 */
    UWORD l = (UWORD)(((ULONG)q * (UWORD)391) >> 13);      /* q / 21 */
    const char *p;
    LONG k = 0;
    for (p = hl[l]; *p; p++) t[k++] = (UBYTE)*p;
    for (p = hv[q - l * 21]; *p; p++) t[k++] = (UBYTE)*p;
    for (p = ht[s - q * 28]; *p; p++) t[k++] = (UBYTE)*p;
    return k;
}

/* c spelt in Latin letters if it is Cyrillic, Greek, kana or Hangul:
 * 1; else 0 */
static int spell(Out *o, Tr *tr, ULONG c)
{
    ULONG b;
    if (c < 0x370) return 0;
    if (c < 0x400 || (c >= 0x1F00 && c < 0x2000)) {        /* Greek */
        b = Utf8NoAccent(c);
        if (b >= 0x391 && b <= 0x3A9 && b != 0x3A2) putcap(o, grk[b - 0x391], 1);
        else if (b >= 0x3B1 && b <= 0x3C9) putcap(o, grk[b - 0x3B1], 0);
        else return 0;
        return 1;
    }
    if (c < 0x500) {                                        /* Cyrillic */
        b = c < 0x460 ? c : Utf8NoAccent(c);
        if (b == 0x490 || b == 0x491) {                     /* Ukrainian ґ */
            putcap(o, "g", b == 0x490);
            return 1;
        }
        if (b >= 0x460) return 0;
        putcap(o, cyr[b - 0x400], b < 0x430);
        return 1;
    }
    if (c < 0x3041) return 0;
    if (c <= 0x30FE) {                                      /* kana */
        if (c <= 0x3096 || (c >= 0x309D && c <= 0x309E) || (c >= 0x30A1 && c <= 0x30FA) ||
            c >= 0x30FC) {
            kanaout(o, tr, c);
            return 1;
        }
        return 0;
    }
    if (c >= 0xAC00 && c < 0xAC00 + 11172) {                /* Hangul syllables */
        UBYTE t[8];
        put(o, t, hangul((UWORD)(c - 0xAC00), t));
        return 1;
    }
    return 0;
}

/* c's stand-in from near[] (sorted: halved), in set cs: 1; else 0 */
static int nearput(Out *o, LONG cs, ULONG c)
{
    LONG a = 0, z = (LONG)(sizeof(near) / sizeof(near[0])) - 1, i, k;
    UBYTE t[4];
    if (c > 0xFFFF) return 0;
    while (a <= z) {
        i = (a + z) >> 1;
        if (c < near[i].c) z = i - 1;
        else if (c > near[i].c) a = i + 1;
        else {
            const UBYTE *s = (const UBYTE *)near[i].s;
            for (k = 0; s[k]; k++) {
                t[k] = setbyte(cs, s[k]);
                if (!t[k]) t[k] = s[k] == 0xB7 ? '*' : '?';
            }
            put(o, t, k);                   /* whole, as every character */
            return 1;
        }
    }
    return 0;
}

static void putset(Out *o, Tr *tr, LONG cs, ULONG c, ULONG flags)
{
    UBYTE t, q = '?';
    ULONG b;
    if (cs == U8CS_UTF8) {
        putc8(o, c);
        return;
    }
    if ((t = setbyte(cs, c)) != 0 || c == 0) {
        put(o, &t, 1);
        return;
    }
    /* invisible: nothing to show */
    if (c == ZWJ || c == 0x200C || c == 0x200B || c == 0xFEFF || c == 0x2060 ||
        (c >= 0xFE00 && c <= 0xFE0F) || (c >= 0xE0000 && c <= 0xE01EF) ||
        (c >= 0x200E && c <= 0x200F) || (c >= 0x202A && c <= 0x202E))
        return;
    if (flags & U8F_STRICT) {
        put(o, &q, 1);
        return;
    }
    if (c >= 0x370 && spell(o, tr, c)) return;
    if (c >= 0x3400 && c < 0xFF01 && plainwide(c)) {        /* Chinese, Japanese kanji */
        put(o, &q, 1);
        return;
    }
    if (c >= 0x1F000) {                     /* emoji and signs */
        if (c >= 0x1F3FB && c <= 0x1F3FF) return;           /* skin tones: nothing */
        if (c >= 0x1F1E6 && c <= 0x1F1FF) t = (UBYTE)('A' + c - 0x1F1E6);  /* flags: SE */
        else t = '?';
        put(o, &t, 1);
        return;
    }
    if (c >= 0x300 && !plainwide(c) && (kind(c) & UC_MARK)) return;     /* an accent left over */
    b = Utf8NoAccent(c);
    if (b != c && (t = setbyte(cs, b)) != 0) {
        put(o, &t, 1);
        return;
    }
    if (c >= 0xFF01 && c <= 0xFF5E) c -= 0xFEE0;            /* fullwidth ASCII */
    else if (c >= 0x2000 && c <= 0x200A) c = ' ';
    else if (c == 0x202F || c == 0x205F) c = ' ';
    if (c < 0x80) {
        t = (UBYTE)c;
        put(o, &t, 1);
        return;
    }
    if (c >= 0x2500 && c <= 0x257F) {
        t = (UBYTE)boxchar(c);
        put(o, &t, 1);
        return;
    }
    if (!nearput(o, cs, c)) put(o, &q, 1);
}

/* the quick way for a whole letter of another script into Latin-1 or
 * ASCII (no set has them, so no set to search): what putset() would do,
 * without its tests for invisible and leftover characters, none of which
 * are here. 0: not one of these, putset() is asked */
static int other(Out *o, Tr *tr, LONG cs, ULONG c)
{
    UBYTE q = '?';
    if (c < 0x388) return 0;
    if (c < 0x3CF) {                                /* Greek */
        if (c >= 0x3B1 && c <= 0x3C9) putcap(o, grk[c - 0x3B1], 0);
        else if (c >= 0x391 && c <= 0x3A9 && c != 0x3A2) putcap(o, grk[c - 0x391], 1);
        else return spell(o, tr, c);                /* with an accent */
        return 1;
    }
    if (c < 0x400) return 0;
    if (c < 0x460) {                                /* Cyrillic */
        putcap(o, cyr[(UWORD)c - 0x400], c < 0x430);
        return 1;
    }
    if (c < 0x2010) return 0;
    if (c < 0x2800) {                               /* punctuation, arrows, signs, boxes */
        /* not the invisible ones (2028-202F, 2060-), marks for signs (20D0-),
         * letterlike signs that are letters (2100-214F, K is K) */
        if ((c >= 0x2028 && c < 0x2030) || (c >= 0x205F && c < 0x2190)) return 0;
        if (c >= 0x2500 && c <= 0x257F) {
            UBYTE b = (UBYTE)boxchar(c);
            put(o, &b, 1);
        } else if (!nearput(o, cs, c))
            put(o, &q, 1);
        return 1;
    }
    if (c < 0x3041) return 0;
    if (c <= 0x30FE) return spell(o, tr, c);       /* kana */
    if (c < 0x3400) return 0;
    if (c < 0xAC00) {
        if (!plainwide(c)) return 0;
        put(o, &q, 1);                              /* Chinese, Japanese kanji */
        return 1;
    }
    if (c < 0xD7A4) return spell(o, tr, c);        /* Hangul */
    if (c < 0x1F000 || c > 0x1FAFF || (c >= 0x1F3FB && c <= 0x1F3FF) || (c >= 0x1F1E6 && c <= 0x1F1FF))
        return 0;
    put(o, &q, 1);                                  /* emoji */
    return 1;
}

/* Utf8ToCharset's tight loop into Latin-1 or ASCII: ASCII, Latin-1's
 * own letters, Cyrillic, Greek, kana, Hangul, Chinese characters, the
 * common punctuation, box lines and emoji, each with nothing joining it, written straight on with the output's place in registers.
 * On a 68000 the way through nextkey() and putset() costs some thousand
 * cycles a character; this a fraction. Stops at anything else, which
 * the full way does (tests/lane.c: both give the same for every code
 * point). Returns where it stopped. */
static __attribute__((noinline)) const UBYTE *run(const UBYTE *p, const UBYTE *e, Out *o, Tr *tr, LONG cs)
{
    UBYTE *w = o->buf ? o->buf + o->w : NULL;
    UBYTE *wend = o->buf ? o->buf + o->size - 1 : NULL;
    LONG n = o->n, full = o->full || !o->buf, k, l;
    int latin = cs == U8CS_LATIN1;
    UBYTE t[8];
    const char *sp;
    while (p < e) {
        UBYTE b = *p;
        if (b < 0x80) {
            /* ASCII: the whole run at once, but its last byte only if what
             * follows cannot join it */
            const UBYTE *q = p + 1;
            while (q < e && *q < 0x80) q++;
            if (q < e && joins(q, e)) q--;
            if (q > p) {
                k = q - p;
                n += k;
                if (!full) {
                    if (w + k <= wend) {
                        do *w++ = *p++; while (p < q);
                        continue;
                    }
                    while (w < wend) *w++ = *p++;
                    full = 1;
                }
                p = q;
                continue;
            }
            l = 1;                          /* an ASCII letter with something after */
            t[0] = b;
            k = 1;
        } else if (b == 0xC2 || b == 0xC3) {    /* Latin-1's own: straight on */
            if (!latin || p + 1 >= e || (p[1] & 0xC0) != 0x80 || (b == 0xC2 && p[1] < 0xA0)) break;
            if (p + 2 < e && joins(p + 2, e)) break;
            if (!full) {
                if (w < wend) *w++ = (UBYTE)(((b & 3) << 6) | (p[1] & 0x3F));
                else full = 1;
            }
            n++;
            p += 2;
            continue;
        } else if (b < 0xD0 || b > 0xEA) {
            if (b == 0xCE || b == 0xCF) {   /* Greek without accents */
                UWORD c;
                if (p + 1 >= e || (p[1] & 0xC0) != 0x80) break;
                c = (UWORD)(((b & 0x1F) << 6) | (p[1] & 0x3F));
                if (c < 0x391 || c > 0x3C9 || c == 0x3A2 || (c > 0x3A9 && c < 0x3B1)) break;
                sp = grk[c >= 0x3B1 ? c - 0x3B1 : c - 0x391];
                for (k = 0; sp[k]; k++) t[k] = (UBYTE)sp[k];
                if (c < 0x3B1) t[0] -= 32;
                l = 2;
            } else if (b == 0xEB || b == 0xEC || (b == 0xED && p + 1 < e && p[1] < 0x9F)) {
                ULONG c;                    /* Hangul, rest of it (EA too, below) */
                if (p + 2 >= e || (p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) break;
                c = ((ULONG)(b & 0x0F) << 12) | ((ULONG)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
                if (c >= 0xD7A4) break;
                k = hangul((UWORD)(c - 0xAC00), t);
                l = 3;
            } else if (b == 0xF0 && p + 3 < e && p[1] == 0x9F && (p[2] & 0xC0) == 0x80 &&
                       (p[3] & 0xC0) == 0x80) {
                ULONG c = 0x1F000 | ((ULONG)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
                /* emoji: '?'; skin tones and flags the full way */
                if (c > 0x1FAFF || (c >= 0x1F3FB && c <= 0x1F3FF) || (c >= 0x1F1E6 && c <= 0x1F1FF)) break;
                t[0] = '?';
                k = 1;
                l = 4;
            } else
                break;
        } else if (b <= 0xD1) {                 /* Cyrillic */
            UWORD c;
            if (p + 1 >= e || (p[1] & 0xC0) != 0x80 || (b == 0xD1 && p[1] >= 0xA0)) break;
            c = (UWORD)(((b & 0x1F) << 6) | (p[1] & 0x3F));
            sp = cyr[c - 0x400];
            for (k = 0; sp[k]; k++) t[k] = (UBYTE)sp[k];
            if (c < 0x430 && t[0] >= 'a' && t[0] <= 'z') t[0] -= 32;
            l = 2;
        } else if (b < 0xE2) {
            break;
        } else {                                /* three bytes: E2-EA */
            UWORD c;
            if (p + 2 >= e || (p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) break;
            c = (UWORD)(((b & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F));
            l = 3;
            if (c >= 0x4E00 && c <= 0x9FFF) {   /* Chinese characters and kanji */
                t[0] = '?';
                k = 1;
            } else if (c >= 0xAC00) {           /* Hangul (from EA) */
                k = hangul((UWORD)(c - 0xAC00), t);
            } else if (c >= 0x2500 && c <= 0x257F) {
                t[0] = (UBYTE)boxtab[c - 0x2500];
                k = 1;
            } else if (c >= 0x2010 && c <= 0x2026) {
                t[0] = (UBYTE)punct[c - 0x2010];
                k = 1;
                if (c == 0x2026) t[1] = t[2] = '.', k = 3;
                else if (t[0] == 0xB7 && !latin) t[0] = '*';
            } else if ((c >= 0x3041 && c <= 0x3096) || c == 0x309D || c == 0x309E ||
                       (c >= 0x30A1 && c <= 0x30FA) || (c >= 0x30FC && c <= 0x30FE)) {
                /* kana: an ordinary one straight on, remembered for the one
                 * after; small ones, the small tsu, the long mark and what
                 * follows a small tsu or n through kanaout() */
                if (p + l < e && joins(p + l, e)) break;
                if (c <= 0x30F6 && c != 0x309D && c != 0x309E) {
                    const char *r = kana[(c >= 0x30A1 ? c - 0x60 : c) - 0x3041];
                    int joined = tr->at == n;
                    if (r[0] != '*' && r[0] != '-' &&
                        !(joined && (tr->tsu || (tr->last[0] == 'n' && !tr->last[1])))) {
                        for (k = 0; k < 4 && r[k]; k++) tr->last[k] = r[k];
                        tr->last[k] = 0;
                        tr->tsu = 0;
                        if (o->buf) tr->w = w - o->buf;
                        if (!full && w + k <= wend) {
                            LONG i;
                            for (i = 0; i < k; i++) *w++ = (UBYTE)r[i];
                        } else
                            full = 1;
                        n += k;
                        tr->at = n;
                        p += l;
                        continue;
                    }
                }
                if (o->buf) {
                    o->w = w - o->buf;
                    o->full = full;
                }
                o->n = n;
                kanaout(o, tr, c);
                if (o->buf) {
                    w = o->buf + o->w;
                    full = o->full;
                }
                n = o->n;
                p += l;
                continue;
            } else
                break;
        }
        if (p + l < e && joins(p + l, e)) break;
        if (!full && w + k <= wend) {
            LONG i;
            for (i = 0; i < k; i++) *w++ = t[i];
        } else
            full = 1;
        n += k;
        p += l;
    }
    if (o->buf) {
        o->w = w - o->buf;
        o->full = full;
    }
    o->n = n;
    return p;
}

LONG Utf8ToCharset(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, LONG cs, ULONG flags)
{
    Out o;
    Keys k;
    Tr tr;
    ULONG c;
    int quicklane;
    outinit(&o, buf, size);
    tr.at = -1;
    if (cs < 0 || cs >= U8CS_COUNT) return -1;
    quicklane = (cs == U8CS_LATIN1 || cs == U8CS_ASCII) && !(flags & U8F_STRICT);
#ifdef NOQUICKLANE
    quicklane = 0;                          /* tests/lane.c: the same without it */
#endif
    if (!s) return outend(&o);
    keysinit(&k, s, len, 0);
    while (k.p < k.e || k.pend) {
        LONG q;
        if (quicklane && !k.pend) {
            k.p = run(k.p, k.e, &o, &tr, cs);
            if (k.p >= k.e) break;
        } else
        /* a run of ASCII straight across */
        if (cs != U8CS_UTF8 && !k.pend) {
            const UBYTE *p = k.p, *e = k.e;
            if (o.buf) {
                UBYTE *w = o.buf + o.w, *wend = o.buf + (o.full ? o.w : o.size - 1);
                while (p < e && *p < 0x80 && (p + 1 == e || p[1] < 0xCC)) {
                    if (w < wend) *w++ = *p;
                    else o.full = 1;
                    p++;
                }
                o.w = w - o.buf;
            } else
                while (p < e && *p < 0x80 && (p + 1 == e || p[1] < 0xCC)) p++;
            o.n += p - k.p;
            k.p = p;
            if (p >= e) break;
        }
        q = k.pend ? 0 : quick(k.p, k.e);
        if (q == 1 && cs != U8CS_UTF8) {    /* ASCII is the same in every set */
            c = *k.p++;
        } else if (q == 2 && k.p[0] < 0xC4 && (cs == U8CS_LATIN1 || cs == U8CS_UTF8)) {
            c = ((ULONG)(k.p[0] & 0x1F) << 6) | (k.p[1] & 0x3F);   /* Latin-1's own */
            k.p += 2;
            if (cs == U8CS_UTF8) {
                putc8(&o, c);
                continue;
            }
        } else {
            if (q == 0 && quicklane && !k.pend && k.p < k.e && k.p[0] >= 0xCE) {
                /* another script's letter with nothing joining it */
                LONG l = getq(k.p, k.e, &c);
                if ((k.p + l >= k.e || !joins(k.p + l, k.e)) && other(&o, &tr, cs, c)) {
                    k.p += l;
                    continue;
                }
            }
            if (!nextkey(&k, &c)) break;
            putset(&o, &tr, cs, c, flags);
            continue;
        }
        if (!o.full && o.w < o.size - 1) o.buf[o.w++] = (UBYTE)c;
        else o.full = 1;
        o.n++;
    }
    return outend(&o);
}

LONG Utf8FromCharset(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, LONG cs)
{
    const UBYTE *p = (const UBYTE *)s, *e;
    ULONG c;
    Out o;
    outinit(&o, buf, size);
    if (cs < 0 || cs >= U8CS_COUNT) return -1;
    if (!s) return outend(&o);
    e = p + slen(s, len);
    while (p < e) {
        if (cs == U8CS_UTF8)
            p += get(p, e, &c);
        else if ((c = *p++) >= 0x80 && !(c = setchar(cs, (UBYTE)c)))
            c = BAD;
        putc8(&o, c);
    }
    return outend(&o);
}

/* the names, the first of each the one Utf8CharsetName gives; matched
 * without case, '-', '_' or spaces */
/* (a digit after \0 would be read as part of it: "\0" "437" apart) */
static const char *const names[U8CS_COUNT] = {
    "ISO-8859-1\0latin1\0l1\0amiga\0iso-ir-100\0cp819\0",
    "ISO-8859-2\0latin2\0l2\0iso-ir-101\0",
    "ISO-8859-15\0latin9\0l9\0latin0\0",
    "ISO-8859-5\0cyrillic\0iso-ir-144\0",
    "ISO-8859-7\0greek\0iso-ir-126\0elot928\0",
    "ISO-8859-9\0latin5\0l5\0turkish\0iso-ir-148\0",
    "windows-1250\0cp1250\0",
    "windows-1251\0cp1251\0",
    "windows-1252\0cp1252\0",
    "IBM437\0cp437\0" "437\0pc8\0dos\0",
    "IBM850\0cp850\0" "850\0",
    "IBM866\0cp866\0" "866\0",
    "KOI8-R\0koi8\0",
    "macintosh\0macroman\0mac\0",
    "US-ASCII\0ascii\0us\0ansix3.41968\0iso646us\0",
    "UTF-8\0utf8\0",
};

static int same(const UBYTE *a, const UBYTE *b)
{
    for (;;) {
        while (*a == '-' || *a == '_' || *a == ' ') a++;
        while (*b == '-' || *b == '_' || *b == ' ') b++;
        if (!*a || !*b) return !*a && !*b;
        if (Utf8Lower(*a) != Utf8Lower(*b)) return 0;
        a++;
        b++;
    }
}

LONG Utf8Charset(CONST_STRPTR name)
{
    LONG i;
    const char *n;
    if (!name) return -1;
    for (i = 0; i < U8CS_COUNT; i++)
        for (n = names[i]; *n; n += strlen(n) + 1)
            if (same((const UBYTE *)name, (const UBYTE *)n)) return i;
    return -1;
}

CONST_STRPTR Utf8CharsetName(LONG cs)
{
    return cs >= 0 && cs < U8CS_COUNT ? names[cs] : NULL;
}
