/* utf8.library: the Resident tag, open/close/expunge, and the jump table.
 * Written by hand, as tui.library's and json.library's (Knowledge:
 * own-shared-library.md): the table below IS the order in utf8.sfd, and
 * that order may never change. The first call is ARexx's (start.s), then
 * the C calls.
 *
 * Every call runs on the calling program's task and keeps nothing, so
 * any number of programs use it at once. Built -nostartfiles with no C
 * library: memcpy and strlen are here. */
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include "port.h"
#include "base.h"
#include "version.h"

#define REG(r, a) a __asm(#r)

struct ExecBase *SysBase;

static const char libname[] = "utf8.library";
static const char libid[] = "utf8.library " UTF8_VERSTR " (" UTF8_DATE ")\r\n";
const char verstag[] = "$VER: utf8.library " UTF8_VERSTR " (" UTF8_DATE ")";

static struct Library *initlib(REG(d0, struct Utf8LibBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys));
static struct Library *libopen(REG(a6, struct Utf8LibBase *b));
static BPTR libclose(REG(a6, struct Utf8LibBase *b));
static BPTR libexpunge(REG(a6, struct Utf8LibBase *b));
static LONG libnull(void);
extern void Utf8RexxEntry(void);   /* start.s */

/* the calls: registers as in utf8.sfd, on to utf8.c */
static LONG L_Decode(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(a1, ULONG *cp))
{ return Utf8Decode(s, len, cp); }
static LONG L_Encode(REG(d0, ULONG cp), REG(a0, STRPTR buf)) { return Utf8Encode(cp, buf); }
static LONG L_Check(REG(a0, CONST_STRPTR s), REG(d0, LONG len)) { return Utf8Check(s, len); }
static LONG L_Valid(REG(a0, CONST_STRPTR s), REG(d0, LONG len)) { return Utf8Valid(s, len); }
static LONG L_Length(REG(a0, CONST_STRPTR s), REG(d0, LONG len)) { return Utf8Length(s, len); }
static LONG L_CharWidth(REG(d0, ULONG cp)) { return Utf8CharWidth(cp); }
static LONG L_Width(REG(a0, CONST_STRPTR s), REG(d0, LONG len)) { return Utf8Width(s, len); }
static LONG L_Fit(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(d1, LONG cols), REG(a1, LONG *w))
{ return Utf8Fit(s, len, cols, w); }
static LONG L_Next(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(d1, LONG pos))
{ return Utf8Next(s, len, pos); }
static LONG L_Prev(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(d1, LONG pos))
{ return Utf8Prev(s, len, pos); }
static ULONG L_Class(REG(d0, ULONG cp)) { return Utf8Class(cp); }
static ULONG L_Upper(REG(d0, ULONG cp)) { return Utf8Upper(cp); }
static ULONG L_Lower(REG(d0, ULONG cp)) { return Utf8Lower(cp); }
static ULONG L_NoAccent(REG(d0, ULONG cp)) { return Utf8NoAccent(cp); }
static LONG L_Compare(REG(a0, CONST_STRPTR a), REG(d0, LONG alen), REG(a1, CONST_STRPTR b),
                      REG(d1, LONG blen), REG(d2, ULONG flags))
{ return Utf8Compare(a, alen, b, blen, flags); }
static LONG L_Find(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(a1, CONST_STRPTR pat),
                   REG(d1, LONG plen), REG(d2, ULONG flags), REG(a2, LONG *ml))
{ return Utf8Find(s, len, pat, plen, flags, ml); }
static LONG L_Case(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(a1, STRPTR buf),
                   REG(d1, LONG size), REG(d2, ULONG flags))
{ return Utf8Case(s, len, buf, size, flags); }
static LONG L_Normalize(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(a1, STRPTR buf),
                        REG(d1, LONG size), REG(d2, ULONG flags))
{ return Utf8Normalize(s, len, buf, size, flags); }
static LONG L_ToCharset(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(a1, STRPTR buf),
                        REG(d1, LONG size), REG(d2, LONG cs), REG(d3, ULONG flags))
{ return Utf8ToCharset(s, len, buf, size, cs, flags); }
static LONG L_FromCharset(REG(a0, CONST_STRPTR s), REG(d0, LONG len), REG(a1, STRPTR buf),
                          REG(d1, LONG size), REG(d2, LONG cs))
{ return Utf8FromCharset(s, len, buf, size, cs); }
static LONG L_Charset(REG(a0, CONST_STRPTR name)) { return Utf8Charset(name); }
static CONST_STRPTR L_CharsetName(REG(d0, LONG cs)) { return Utf8CharsetName(cs); }

/* in utf8.sfd's order, after exec's four: never reorder, only add */
static const APTR functable[] = {
    (APTR)libopen, (APTR)libclose, (APTR)libexpunge, (APTR)libnull,
    /* 0.1 */
    (APTR)Utf8RexxEntry,
    (APTR)L_Decode, (APTR)L_Encode, (APTR)L_Check, (APTR)L_Valid,
    (APTR)L_Length, (APTR)L_CharWidth, (APTR)L_Width, (APTR)L_Fit,
    (APTR)L_Next, (APTR)L_Prev, (APTR)L_Class, (APTR)L_Upper,
    (APTR)L_Lower, (APTR)L_NoAccent, (APTR)L_Compare, (APTR)L_Find,
    (APTR)L_Case, (APTR)L_Normalize, (APTR)L_ToCharset, (APTR)L_FromCharset,
    (APTR)L_Charset, (APTR)L_CharsetName,
    (APTR)-1
};

static const ULONG inittab[4] = {
    sizeof(struct Utf8LibBase), (ULONG)functable, 0, (ULONG)initlib
};

const struct Resident romtag = {
    RTC_MATCHWORD, (struct Resident *)&romtag, (APTR)(&romtag + 1),
    RTF_AUTOINIT, UTF8_VERSION, NT_LIBRARY, 0,
    (char *)libname, (char *)libid, (APTR)inittab
};

static struct Library *initlib(REG(d0, struct Utf8LibBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys))
{
    SysBase = sys;
    b->lib.lib_Node.ln_Type = NT_LIBRARY;
    b->lib.lib_Node.ln_Name = (char *)libname;
    b->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    b->lib.lib_Version = UTF8_VERSION;
    b->lib.lib_Revision = UTF8_REVISION;
    b->lib.lib_IdString = (APTR)libid;
    b->seglist = seg;
    InitSemaphore(&b->lock);
    return &b->lib;
}

static struct Library *libopen(REG(a6, struct Utf8LibBase *b))
{
    b->lib.lib_OpenCnt++;
    b->lib.lib_Flags &= ~LIBF_DELEXP;
    return &b->lib;
}

static BPTR libexpunge(REG(a6, struct Utf8LibBase *b))
{
    BPTR seg;
    if (b->lib.lib_OpenCnt) {
        b->lib.lib_Flags |= LIBF_DELEXP;   /* when the last one closes */
        return 0;
    }
    seg = b->seglist;
    Remove(&b->lib.lib_Node);
    rexx_close();
    FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
    return seg;
}

static BPTR libclose(REG(a6, struct Utf8LibBase *b))
{
    if (--b->lib.lib_OpenCnt == 0 && (b->lib.lib_Flags & LIBF_DELEXP))
        return libexpunge(b);
    return 0;
}

static LONG libnull(void)
{
    return 0;
}

/* no C library in here: what gcc and utf8.c call */
void *memcpy(void *d, const void *s, unsigned long n)
{
    UBYTE *a = d;
    const UBYTE *b = s;
    while (n--) *a++ = *b++;
    return d;
}

void *memset(void *d, int c, unsigned long n)
{
    UBYTE *a = d;
    while (n--) *a++ = (UBYTE)c;
    return d;
}

unsigned long strlen(const char *s)
{
    const char *e = s;
    while (*e) e++;
    return e - s;
}
