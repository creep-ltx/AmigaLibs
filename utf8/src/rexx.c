/* rexx.c - utf8.library as an ARexx function library.
 *
 *   CALL ADDLIB('utf8.library', 0, -30, 0)
 *
 * ARexx calls the library's first entry (offset -30, Utf8RexxEntry in
 * start.s) with a RexxMsg for every function it does not know; the
 * function's name is ARG0. A name that is not ours gets error 1, so
 * ARexx asks the next library.
 *
 * ARexx's own LENGTH, SUBSTR, LEFT, POS and UPPER count bytes and know
 * only Latin-1; these count whole characters and know all of Unicode.
 * Positions are characters, from 1, as in ARexx. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <stddef.h>
#include "port.h"
#include "base.h"

struct RxsLib *RexxSysBase;

#define ALEN(a) (((struct RexxArg *)((UBYTE *)(a) - offsetof(struct RexxArg, ra_Buff)))->ra_Length)
#define MAXREXX 65535               /* the longest ARexx string */

/* ---- small helpers ----------------------------------------------------- */

static int upper(int c)
{
    return c >= 'a' && c <= 'z' ? c - 32 : c;
}

static int same(const UBYTE *a, const char *b)
{
    while (*a && *b)
        if (upper(*a++) != *b++) return 0;
    return !*a && !*b;
}

/* is word in the options text (any case)? */
static int hasopt(const UBYTE *s, const char *word)
{
    int k = strlen(word), i;
    if (!s) return 0;
    for (; *s; s++) {
        for (i = 0; i < k && s[i] && upper(s[i]) == word[i]; i++)
            ;
        if (i == k) return 1;
    }
    return 0;
}

static ULONG flagsof(const UBYTE *opts)
{
    return (hasopt(opts, "NOCASE") ? U8F_NOCASE : 0) | (hasopt(opts, "NOACCENT") ? U8F_NOACCENT : 0);
}

/* a whole number: 123, or hex as U+00E5, 0xE5 or $E5; -1 if none */
static LONG number(const UBYTE *s)
{
    LONG v = 0, hex = 0, any = 0;
    if (!s) return -1;
    while (*s == ' ') s++;
    if ((upper(s[0]) == 'U' && s[1] == '+') || (s[0] == '0' && upper(s[1]) == 'X')) {
        s += 2;
        hex = 1;
    } else if (*s == '$') {
        s++;
        hex = 1;
    }
    for (;; s++) {
        int c = upper(*s), d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (hex && c >= 'A' && c <= 'F') d = c - 'A' + 10;
        else break;
        if (v > 0x7FFFFFF) return -1;
        v = hex ? (v << 4) + d : (v << 3) + (v << 1) + d;
        any = 1;
    }
    while (*s == ' ') s++;
    return any && !*s ? v : -1;
}

static int itoa(LONG v, char *b)
{
    static const ULONG p10[] = { 1000000000UL, 100000000UL, 10000000UL, 1000000UL,
                                 100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL };
    ULONG u = v < 0 ? 0 - (ULONG)v : (ULONG)v;
    int k = 0, i, on = 0;
    if (v < 0) b[k++] = '-';
    for (i = 0; i < 10; i++) {
        char c = '0';
        while (u >= p10[i]) {
            u -= p10[i];
            c++;
        }
        if (c != '0' || on || i == 9) {
            b[k++] = c;
            on = 1;
        }
    }
    b[k] = 0;
    return k;
}

static LONG result(UBYTE **res, const char *s, LONG len)
{
    if (len < 0) len = strlen(s);
    if (len > MAXREXX) len = MAXREXX;
    *res = CreateArgstring((CONST_STRPTR)s, len);
    return *res ? 0 : ERR10_003;
}

static LONG numresult(UBYTE **res, LONG v)
{
    char num[12];
    return result(res, num, itoa(v, num));
}

/* the bytes of characters from (1 = the first) for count (-1: the rest) */
static void span(const UBYTE *s, LONG len, LONG from, LONG count, LONG *p, LONG *q)
{
    LONG i;
    *p = 0;
    for (i = 1; i < from && *p < len; i++) *p = Utf8Next((CONST_STRPTR)s, len, *p);
    *q = *p;
    for (i = 0; (count < 0 || i < count) && *q < len; i++) *q = Utf8Next((CONST_STRPTR)s, len, *q);
}

/* a call that writes into a buffer: measured, then written */
typedef LONG (*Writer)(const UBYTE *s, LONG len, STRPTR buf, LONG size, LONG arg);

static LONG w_case(const UBYTE *s, LONG len, STRPTR buf, LONG size, LONG arg)
{ return Utf8Case((CONST_STRPTR)s, len, buf, size, arg); }
static LONG w_to(const UBYTE *s, LONG len, STRPTR buf, LONG size, LONG arg)
{ return Utf8ToCharset((CONST_STRPTR)s, len, buf, size, arg, 0); }
static LONG w_from(const UBYTE *s, LONG len, STRPTR buf, LONG size, LONG arg)
{ return Utf8FromCharset((CONST_STRPTR)s, len, buf, size, arg); }
static LONG w_norm(const UBYTE *s, LONG len, STRPTR buf, LONG size, LONG arg)
{ return Utf8Normalize((CONST_STRPTR)s, len, buf, size, arg); }

static LONG written(UBYTE **res, Writer f, const UBYTE *s, LONG len, LONG arg)
{
    LONG n = f(s, len, NULL, 0, arg), rc;
    STRPTR t;
    if (n < 0) return ERR10_018;
    if (n > MAXREXX) n = MAXREXX;
    if (!(t = AllocMem(n + 1, MEMF_ANY))) return ERR10_003;   /* (not AllocVec: 1.3 has none) */
    f(s, len, t, n + 1, arg);
    rc = result(res, t, strlen(t) < (ULONG)n ? (LONG)strlen(t) : n);
    FreeMem(t, n + 1);
    return rc;
}

void rexx_close(void)
{
    if (RexxSysBase) {
        CloseLibrary((struct Library *)RexxSysBase);
        RexxSysBase = NULL;
    }
}

/* called from Utf8RexxEntry (start.s): an ARexx error code back, and the
 * result string in *res */
LONG rexx_query(struct Utf8LibBase *b, struct RexxMsg *m, UBYTE **res)
{
    const UBYTE *fn = (const UBYTE *)m->rm_Args[0], *a1, *a2, *a3;
    int nargs = m->rm_Action & RXARGMASK, i;
    LONG l1, l2, p, q, v, ml;

    *res = NULL;
    if (!fn || upper(fn[0]) != 'U' || upper(fn[1]) != 'T' || upper(fn[2]) != 'F' || fn[3] != '8')
        return ERR10_001;           /* not ours: the next library */
    if (!RexxSysBase) {
        ObtainSemaphore(&b->lock);
        if (!RexxSysBase) RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 36);
        ReleaseSemaphore(&b->lock);
        if (!RexxSysBase) return ERR10_003;
    }
    a1 = nargs >= 1 ? (const UBYTE *)m->rm_Args[1] : NULL;
    a2 = nargs >= 2 ? (const UBYTE *)m->rm_Args[2] : NULL;
    a3 = nargs >= 3 ? (const UBYTE *)m->rm_Args[3] : NULL;
    l1 = a1 ? ALEN(a1) : 0;
    l2 = a2 ? ALEN(a2) : 0;
    fn += 4;

    if (same(fn, "CHAR")) {                 /* UTF8CHAR(code, [code...]) */
        char t[15 * 4];
        LONG k = 0;
        if (!nargs) return ERR10_017;
        for (i = 1; i <= nargs; i++) {
            v = number((const UBYTE *)m->rm_Args[i]);
            if (v < 0 || !(p = Utf8Encode(v, t + k))) return ERR10_018;
            k += p;
        }
        return result(res, t, k);
    }

    /* the rest take a text first */
    if (!a1) return ERR10_017;

    if (same(fn, "LEN"))                    /* UTF8LEN(text) */
        return numresult(res, Utf8Length((CONST_STRPTR)a1, l1));
    if (same(fn, "WIDTH"))                  /* UTF8WIDTH(text) */
        return numresult(res, Utf8Width((CONST_STRPTR)a1, l1));
    if (same(fn, "LEFT")) {                 /* UTF8LEFT(text, columns) */
        if ((v = number(a2)) < 0) return ERR10_018;
        return result(res, (const char *)a1, Utf8Fit((CONST_STRPTR)a1, l1, v, NULL));
    }
    if (same(fn, "SUBSTR")) {               /* UTF8SUBSTR(text, start, [count]) */
        LONG count = a3 ? number(a3) : -1;
        if ((v = number(a2)) < 1 || (a3 && count < 0)) return ERR10_018;
        span(a1, l1, v, count, &p, &q);
        return result(res, (const char *)a1 + p, q - p);
    }
    if (same(fn, "UPPER"))                  /* UTF8UPPER(text) */
        return written(res, w_case, a1, l1, U8F_UPPER);
    if (same(fn, "LOWER"))                  /* UTF8LOWER(text) */
        return written(res, w_case, a1, l1, U8F_LOWER);
    if (same(fn, "TO") || same(fn, "FROM")) {   /* UTF8TO(text, [set]), UTF8FROM(text, [set]) */
        LONG cs = a2 ? Utf8Charset((CONST_STRPTR)a2) : U8CS_LATIN1;
        if (cs < 0) return ERR10_018;
        return written(res, fn[0] == 'T' || fn[0] == 't' ? w_to : w_from, a1, l1, cs);
    }
    if (same(fn, "NORMALIZE"))              /* UTF8NORMALIZE(text, [NFD]) */
        return written(res, w_norm, a1, l1, hasopt(a2, "NFD") ? U8F_DECOMPOSE : U8F_COMPOSE);
    if (same(fn, "CHECK")) {                /* UTF8CHECK(text) */
        static const char *const kinds[] = { "ASCII", "UTF8", "LATIN1" };
        return result(res, kinds[Utf8Check((CONST_STRPTR)a1, l1)], -1);
    }
    if (same(fn, "CODE")) {                 /* UTF8CODE(text, [n]): the n-th code point */
        ULONG c = 0;
        LONG at = 0, k = 1, n = a2 ? number(a2) : 1;
        if (n < 1) return ERR10_018;
        while (at < l1) {
            LONG used = Utf8Decode((CONST_STRPTR)a1 + at, l1 - at, &c);
            if (k++ == n) return numresult(res, (LONG)c);
            at += used < 0 ? 1 : used;
        }
        return result(res, "", 0);
    }
    if (same(fn, "FIND")) {                 /* UTF8FIND(text, find, [options]) */
        LONG n = 0;
        if (!a2) return ERR10_017;
        v = Utf8Find((CONST_STRPTR)a1, l1, (CONST_STRPTR)a2, l2, flagsof(a3), &ml);
        if (v < 0) return result(res, "0", 1);
        for (p = 0; p < v; p = Utf8Next((CONST_STRPTR)a1, l1, p)) n++;
        return numresult(res, n + 1);
    }
    if (same(fn, "COMPARE")) {              /* UTF8COMPARE(a, b, [options]) */
        if (!a2) return ERR10_017;
        v = Utf8Compare((CONST_STRPTR)a1, l1, (CONST_STRPTR)a2, l2, flagsof(a3));
        return numresult(res, v < 0 ? -1 : v > 0 ? 1 : 0);
    }
    return ERR10_001;
}
