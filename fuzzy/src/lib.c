/* fuzzy.library: the Resident tag, open/close/expunge, and the jump table.
 * Written by hand, as tui.library's (Knowledge: own-shared-library.md):
 * the table below IS the order in fuzzy.sfd, and that order may never
 * change. The first call is ARexx's (start.s), then the C calls.
 *
 * Every call runs on the calling program's task; patterns, indexes and
 * results belong to whoever made them: the library keeps nothing for them. Built
 * -nostartfiles with no C library: memcpy and friends are here. */
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include "port.h"
#include "base.h"
#include "version.h"

#define REG(r, a) a __asm(#r)

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

static const char libname[] = "fuzzy.library";
static const char libid[] = "fuzzy.library " FUZZY_VERSTR " (" FUZZY_DATE ")\r\n";
const char verstag[] = "$VER: fuzzy.library " FUZZY_VERSTR " (" FUZZY_DATE ")";

static struct Library *initlib(REG(d0, struct FuzzyBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys));
static struct Library *libopen(REG(a6, struct FuzzyBase *b));
static BPTR libclose(REG(a6, struct FuzzyBase *b));
static BPTR libexpunge(REG(a6, struct FuzzyBase *b));
static LONG libnull(void);
extern void FuzzyRexxEntry(void);   /* start.s */

/* the calls: registers as in fuzzy.sfd, on to match.c and index.c */
static FuzzyPattern *L_Prepare(REG(a0, CONST_STRPTR s), REG(d0, ULONG fl)) { return FuzzyPrepare(s, fl); }
static void L_FreePattern(REG(a0, FuzzyPattern *p)) { FuzzyFreePattern(p); }
static LONG L_Score(REG(a0, FuzzyPattern *p), REG(a1, CONST_STRPTR t), REG(d0, LONG len), REG(a2, UWORD *pos),
                    REG(d1, LONG maxpos))
{ return FuzzyScore(p, t, len, pos, maxpos); }
static LONG L_IndexPath(REG(a0, STRPTR b), REG(d0, LONG size)) { return FuzzyIndexPath(b, size); }
static struct FuzzyIndex *L_OpenIndex(REG(a0, CONST_STRPTR n), REG(a1, LONG *err)) { return FuzzyOpenIndex(n, err); }
static void L_CloseIndex(REG(a0, struct FuzzyIndex *x)) { FuzzyCloseIndex(x); }
static LONG L_NextEntry(REG(a0, struct FuzzyIndex *x), REG(a1, struct FuzzyEntry *e)) { return FuzzyNextEntry(x, e); }
static void L_Rewind(REG(a0, struct FuzzyIndex *x)) { FuzzyRewind(x); }
static CONST_STRPTR L_Volume(REG(a0, struct FuzzyIndex *x), REG(d0, LONG n), REG(a1, ULONG *date))
{ return FuzzyVolume(x, n, date); }
static struct FuzzyResults *L_Search(REG(a0, struct FuzzyIndex *x), REG(a1, CONST_STRPTR p), REG(d0, ULONG fl),
                                     REG(d1, LONG max))
{ return FuzzySearch(x, p, fl, max); }
static void L_FreeResults(REG(a0, struct FuzzyResults *r)) { FuzzyFreeResults(r); }
static FuzzyWriter *L_NewIndex(REG(a0, CONST_STRPTR n), REG(a1, LONG *err)) { return FuzzyNewIndex(n, err); }
static LONG L_AddVolume(REG(a0, FuzzyWriter *w), REG(a1, CONST_STRPTR n), REG(d0, ULONG date))
{ return FuzzyAddVolume(w, n, date); }
static LONG L_AddEntry(REG(a0, FuzzyWriter *w), REG(a1, struct FuzzyEntry *e)) { return FuzzyAddEntry(w, e); }
static LONG L_EndIndex(REG(a0, FuzzyWriter *w), REG(d0, LONG keep)) { return FuzzyEndIndex(w, keep); }
static CONST_STRPTR L_ErrorText(REG(d0, LONG e)) { return FuzzyErrorText(e); }

/* in fuzzy.sfd's order, after exec's four: never reorder, only add */
static const APTR functable[] = {
    (APTR)libopen, (APTR)libclose, (APTR)libexpunge, (APTR)libnull,
    /* 0.1 */
    (APTR)FuzzyRexxEntry,
    (APTR)L_Prepare, (APTR)L_FreePattern, (APTR)L_Score, (APTR)L_IndexPath,
    (APTR)L_OpenIndex, (APTR)L_CloseIndex, (APTR)L_NextEntry, (APTR)L_Rewind,
    (APTR)L_Volume, (APTR)L_Search, (APTR)L_FreeResults, (APTR)L_NewIndex,
    (APTR)L_AddVolume, (APTR)L_AddEntry, (APTR)L_EndIndex, (APTR)L_ErrorText,
    (APTR)-1
};

static const ULONG inittab[4] = {
    sizeof(struct FuzzyBase), (ULONG)functable, 0, (ULONG)initlib
};

const struct Resident romtag = {
    RTC_MATCHWORD, (struct Resident *)&romtag, (APTR)(&romtag + 1),
    RTF_AUTOINIT, FUZZY_VERSION, NT_LIBRARY, 0,
    (char *)libname, (char *)libid, (APTR)inittab
};

static struct Library *initlib(REG(d0, struct FuzzyBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys))
{
    SysBase = sys;
    b->lib.lib_Node.ln_Type = NT_LIBRARY;
    b->lib.lib_Node.ln_Name = (char *)libname;
    b->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    b->lib.lib_Version = FUZZY_VERSION;
    b->lib.lib_Revision = FUZZY_REVISION;
    b->lib.lib_IdString = (APTR)libid;
    b->seglist = seg;
    InitSemaphore(&b->lock);
    b->lasterr[0] = 0;
    if (!(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37))) {
        FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
        return NULL;
    }
    return &b->lib;
}

static struct Library *libopen(REG(a6, struct FuzzyBase *b))
{
    b->lib.lib_OpenCnt++;
    b->lib.lib_Flags &= ~LIBF_DELEXP;
    return &b->lib;
}

static BPTR libexpunge(REG(a6, struct FuzzyBase *b))
{
    BPTR seg;
    if (b->lib.lib_OpenCnt) {
        b->lib.lib_Flags |= LIBF_DELEXP;   /* when the last one closes */
        return 0;
    }
    seg = b->seglist;
    Remove(&b->lib.lib_Node);
    rexx_close(b);
    CloseLibrary((struct Library *)DOSBase);
    FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
    return seg;
}

static BPTR libclose(REG(a6, struct FuzzyBase *b))
{
    if (--b->lib.lib_OpenCnt == 0 && (b->lib.lib_Flags & LIBF_DELEXP))
        return libexpunge(b);
    return 0;
}

static LONG libnull(void)
{
    return 0;
}

/* no C library in here: what gcc and the core call */
void *memcpy(void *d, const void *s, unsigned long n)
{
    UBYTE *a = d;
    const UBYTE *b = s;
    while (n--) *a++ = *b++;
    return d;
}

void *memmove(void *d, const void *s, unsigned long n)
{
    UBYTE *a = d;
    const UBYTE *b = s;
    if (a < b) while (n--) *a++ = *b++;
    else { a += n; b += n; while (n--) *--a = *--b; }
    return d;
}

void *memset(void *d, int c, unsigned long n)
{
    UBYTE *a = d;
    while (n--) *a++ = (UBYTE)c;
    return d;
}

int memcmp(const void *x, const void *y, unsigned long n)
{
    const UBYTE *a = x, *b = y;
    for (; n; n--, a++, b++)
        if (*a != *b) return *a - *b;
    return 0;
}

unsigned long strlen(const char *s)
{
    const char *e = s;
    while (*e) e++;
    return e - s;
}
