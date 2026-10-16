/* rexx.c - fuzzy.library as an ARexx function library.
 *
 *   CALL ADDLIB('fuzzy.library', 0, -30, 0)
 *   SAY FUZZYSCORE('csrc', 'config/source.c')      -> a number, 0 = no match
 *   SAY FUZZYLOCATE('dpaint', 5)                   -> the 5 best paths of the
 *                                                     index, one a line
 *   SAY FUZZYERROR()                               -> why the last one failed
 *
 * Options (any of, any case, in one string): PLAIN CASE ACCENTS TEXT, and
 * for FUZZYLOCATE also FILES DRAWERS NAME. ARexx calls FuzzyRexxEntry in
 * start.s, which returns the result in A0 and A1 (json.library found
 * ARexx reading A0). */
#include <exec/types.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <stddef.h>
#include "port.h"
#include "base.h"

struct RxsLib *RexxSysBase;

#define ALEN(a) (((struct RexxArg *)((UBYTE *)(a) - offsetof(struct RexxArg, ra_Buff)))->ra_Length)
#define MAXREXX 65535

static int upper(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7) ? c - 32 : c;
}

static int same(const UBYTE *a, const char *b)
{
    while (*a && *b)
        if (upper(*a++) != *b++) return 0;
    return !*a && !*b;
}

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

static ULONG options(const UBYTE *o)
{
    return (hasopt(o, "PLAIN") ? FPF_PLAIN : 0) | (hasopt(o, "CASE") ? FPF_CASE : 0) |
           (hasopt(o, "ACCENTS") ? FPF_ACCENTS : 0) | (hasopt(o, "TEXT") ? FPF_TEXT : 0) |
           (hasopt(o, "FILES") ? FSF_FILES : 0) | (hasopt(o, "DRAWERS") ? FSF_DRAWERS : 0) |
           (hasopt(o, "NAME") ? FSF_NAME : 0);
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

static void seterr(struct FuzzyBase *b, const char *msg)
{
    int k = 0;
    while (*msg && k < (int)sizeof(b->lasterr) - 1) b->lasterr[k++] = *msg++;
    b->lasterr[k] = 0;
}

static LONG result(UBYTE **res, const char *s, LONG len)
{
    if (len < 0) len = strlen(s);
    if (len > MAXREXX) len = MAXREXX;
    *res = CreateArgstring((CONST_STRPTR)s, len);
    return *res ? 0 : ERR10_003;
}

void rexx_close(struct FuzzyBase *b)
{
    (void)b;
    if (RexxSysBase) {
        CloseLibrary((struct Library *)RexxSysBase);
        RexxSysBase = NULL;
    }
}

LONG rexx_query(struct FuzzyBase *b, struct RexxMsg *m, UBYTE **res)
{
    const UBYTE *fn = (const UBYTE *)m->rm_Args[0], *a1, *a2, *a3;
    int nargs = m->rm_Action & RXARGMASK;
    char num[12];

    *res = NULL;
    if (!fn || upper(fn[0]) != 'F' || upper(fn[1]) != 'U' || upper(fn[2]) != 'Z' ||
        upper(fn[3]) != 'Z' || upper(fn[4]) != 'Y')
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
    fn += 5;

    if (same(fn, "SCORE")) {                /* FUZZYSCORE(pattern, text, [options]) */
        FuzzyPattern *p;
        LONG sc;
        if (!a1 || !a2) return ERR10_017;
        if (!(p = FuzzyPrepare((CONST_STRPTR)a1, options(a3) & 0xFF))) return ERR10_003;
        sc = FuzzyScore(p, (CONST_STRPTR)a2, ALEN(a2), NULL, 0);
        FuzzyFreePattern(p);
        return result(res, num, itoa(sc, num));
    }
    if (same(fn, "LOCATE")) {               /* FUZZYLOCATE(pattern, [max], [options]) */
        struct FuzzyIndex *x;
        struct FuzzyResults *r;
        LONG err, max = 20, i, len = 0, rc;
        char *out, *o;
        if (!a1) return ERR10_017;
        if (a2 && *a2) {
            const UBYTE *s = a2;
            for (max = 0; *s >= '0' && *s <= '9'; s++)
                if (max < 100000) max = (max << 3) + (max << 1) + (*s - '0');
            if (*s || max < 1) return ERR10_018;
        }
        if (!(x = FuzzyOpenIndex(NULL, &err))) {
            seterr(b, (const char *)FuzzyErrorText(err));
            return result(res, "", 0);
        }
        r = FuzzySearch(x, (CONST_STRPTR)a1, options(a3), max);
        FuzzyCloseIndex(x);
        if (!r) return ERR10_003;
        for (i = 0; i < r->Count; i++) len += r->Hits[i].PathLen + 1;
        if (!(out = AllocVec(len + 1, MEMF_ANY))) {
            FuzzyFreeResults(r);
            return ERR10_003;
        }
        for (o = out, i = 0; i < r->Count; i++) {
            if (i) *o++ = '\n';
            memcpy(o, r->Hits[i].Path, r->Hits[i].PathLen);
            o += r->Hits[i].PathLen;
        }
        if (o - out > MAXREXX) {
            seterr(b, "too long for an ARexx string: ask for fewer");
            rc = result(res, "", 0);
        } else {
            b->lasterr[0] = 0;
            rc = result(res, out, o - out);
        }
        FreeVec(out);
        FuzzyFreeResults(r);
        return rc;
    }
    if (same(fn, "ERROR"))                  /* FUZZYERROR() */
        return result(res, b->lasterr, -1);
    return ERR10_001;
}
