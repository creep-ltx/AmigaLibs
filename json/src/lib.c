/* json.library: the Resident tag, open/close/expunge, and the jump table.
 * Written by hand, as tui.library's (Knowledge: own-shared-library.md):
 * the table below IS the order in json.sfd, and that order may never
 * change. The first call is ARexx's (start.s), then the C calls.
 *
 * Every call runs on the calling program's task; documents belong to
 * whoever made them, so the library keeps nothing for C programs. Built
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

static const char libname[] = "json.library";
static const char libid[] = "json.library " JSON_VERSTR " (" JSON_DATE ")\r\n";
const char verstag[] = "$VER: json.library " JSON_VERSTR " (" JSON_DATE ")";

static struct Library *initlib(REG(d0, struct JsonBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys));
static struct Library *libopen(REG(a6, struct JsonBase *b));
static BPTR libclose(REG(a6, struct JsonBase *b));
static BPTR libexpunge(REG(a6, struct JsonBase *b));
static LONG libnull(void);
extern void JsonRexxEntry(void);   /* start.s */

/* the calls: registers as in json.sfd, on to json.c */
static JsonDoc *L_Parse(REG(a0, CONST_STRPTR t), REG(d0, LONG len), REG(d1, ULONG fl), REG(a1, struct JsonError *e))
{ return JsonParse(t, len, fl, e); }
static JsonDoc *L_Load(REG(a0, CONST_STRPTR name), REG(d0, ULONG fl), REG(a1, struct JsonError *e))
{ return JsonLoad(name, fl, e); }
static void L_Free(REG(a0, JsonDoc *d)) { JsonFree(d); }
static JsonNode *L_Root(REG(a0, JsonDoc *d)) { return JsonRoot(d); }
static LONG L_Type(REG(a0, JsonNode *n)) { return JsonType(n); }
static JsonNode *L_Get(REG(a0, JsonNode *o), REG(a1, CONST_STRPTR name)) { return JsonGet(o, name); }
static JsonNode *L_At(REG(a0, JsonNode *a), REG(d0, LONG i)) { return JsonAt(a, i); }
static JsonNode *L_Find(REG(a0, JsonNode *n), REG(a1, CONST_STRPTR p)) { return JsonFind(n, p); }
static LONG L_Count(REG(a0, JsonNode *n)) { return JsonCount(n); }
static JsonNode *L_First(REG(a0, JsonNode *n)) { return JsonFirst(n); }
static JsonNode *L_Next(REG(a0, JsonNode *n)) { return JsonNext(n); }
static JsonNode *L_Parent(REG(a0, JsonNode *n)) { return JsonParent(n); }
static CONST_STRPTR L_Name(REG(a0, JsonNode *n)) { return JsonName(n); }
static CONST_STRPTR L_String(REG(a0, JsonNode *n)) { return JsonString(n); }
static LONG L_Length(REG(a0, JsonNode *n)) { return JsonLength(n); }
static CONST_STRPTR L_NumText(REG(a0, JsonNode *n)) { return JsonNumText(n); }
static LONG L_Int(REG(a0, JsonNode *n), REG(d0, LONG def)) { return JsonInt(n, def); }
static LONG L_Scaled(REG(a0, JsonNode *n), REG(d0, LONG places), REG(d1, LONG def))
{ return JsonScaled(n, places, def); }
static LONG L_Bool(REG(a0, JsonNode *n), REG(d0, LONG def)) { return JsonBool(n, def); }
static LONG L_Latin1(REG(a0, JsonNode *n), REG(a1, STRPTR buf), REG(d0, LONG size))
{ return JsonLatin1(n, buf, size); }
static JsonDoc *L_NewDoc(void) { return JsonNewDoc(); }
static void L_SetRoot(REG(a0, JsonDoc *d), REG(a1, JsonNode *n)) { JsonSetRoot(d, n); }
static JsonNode *L_NewObject(REG(a0, JsonDoc *d)) { return JsonNewObject(d); }
static JsonNode *L_NewArray(REG(a0, JsonDoc *d)) { return JsonNewArray(d); }
static JsonNode *L_NewString(REG(a0, JsonDoc *d), REG(a1, CONST_STRPTR s), REG(d0, LONG len), REG(d1, ULONG fl))
{ return JsonNewString(d, s, len, fl); }
static JsonNode *L_NewInt(REG(a0, JsonDoc *d), REG(d0, LONG v)) { return JsonNewInt(d, v); }
static JsonNode *L_NewNumber(REG(a0, JsonDoc *d), REG(a1, CONST_STRPTR t)) { return JsonNewNumber(d, t); }
static JsonNode *L_NewBool(REG(a0, JsonDoc *d), REG(d0, LONG v)) { return JsonNewBool(d, v); }
static JsonNode *L_NewNull(REG(a0, JsonDoc *d)) { return JsonNewNull(d); }
static LONG L_Add(REG(a0, JsonNode *p), REG(a1, CONST_STRPTR name), REG(a2, JsonNode *n))
{ return JsonAdd(p, name, n); }
static void L_Remove(REG(a0, JsonNode *n)) { JsonRemove(n); }
static LONG L_Write(REG(a0, JsonNode *n), REG(a1, STRPTR buf), REG(d0, LONG size), REG(d1, ULONG fl))
{ return JsonWrite(n, buf, size, fl); }
static LONG L_WriteFH(REG(a0, JsonNode *n), REG(d1, BPTR fh), REG(d0, ULONG fl))
{ return JsonWriteFH(n, fh, fl); }

/* in json.sfd's order, after exec's four: never reorder, only add */
static const APTR functable[] = {
    (APTR)libopen, (APTR)libclose, (APTR)libexpunge, (APTR)libnull,
    /* 0.1 */
    (APTR)JsonRexxEntry,
    (APTR)L_Parse, (APTR)L_Load, (APTR)L_Free, (APTR)L_Root,
    (APTR)L_Type, (APTR)L_Get, (APTR)L_At, (APTR)L_Find,
    (APTR)L_Count, (APTR)L_First, (APTR)L_Next, (APTR)L_Parent,
    (APTR)L_Name, (APTR)L_String, (APTR)L_Length, (APTR)L_NumText,
    (APTR)L_Int, (APTR)L_Scaled, (APTR)L_Bool, (APTR)L_Latin1,
    (APTR)L_NewDoc, (APTR)L_SetRoot, (APTR)L_NewObject, (APTR)L_NewArray,
    (APTR)L_NewString, (APTR)L_NewInt, (APTR)L_NewNumber, (APTR)L_NewBool,
    (APTR)L_NewNull, (APTR)L_Add, (APTR)L_Remove, (APTR)L_Write,
    (APTR)L_WriteFH,
    (APTR)-1
};

static const ULONG inittab[4] = {
    sizeof(struct JsonBase), (ULONG)functable, 0, (ULONG)initlib
};

const struct Resident romtag = {
    RTC_MATCHWORD, (struct Resident *)&romtag, (APTR)(&romtag + 1),
    RTF_AUTOINIT, JSON_VERSION, NT_LIBRARY, 0,
    (char *)libname, (char *)libid, (APTR)inittab
};

static struct Library *initlib(REG(d0, struct JsonBase *b), REG(a0, BPTR seg), REG(a6, struct ExecBase *sys))
{
    SysBase = sys;
    b->lib.lib_Node.ln_Type = NT_LIBRARY;
    b->lib.lib_Node.ln_Name = (char *)libname;
    b->lib.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    b->lib.lib_Version = JSON_VERSION;
    b->lib.lib_Revision = JSON_REVISION;
    b->lib.lib_IdString = (APTR)libid;
    b->seglist = seg;
    InitSemaphore(&b->lock);
    b->handles.mlh_Head = (struct MinNode *)&b->handles.mlh_Tail;   /* NewList (amiga.lib) */
    b->handles.mlh_Tail = NULL;
    b->handles.mlh_TailPred = (struct MinNode *)&b->handles.mlh_Head;
    b->nextid = 0;
    b->lasterr[0] = 0;
    if (!(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37))) {
        FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
        return NULL;
    }
    return &b->lib;
}

static struct Library *libopen(REG(a6, struct JsonBase *b))
{
    b->lib.lib_OpenCnt++;
    b->lib.lib_Flags &= ~LIBF_DELEXP;
    return &b->lib;
}

static BPTR libexpunge(REG(a6, struct JsonBase *b))
{
    BPTR seg;
    if (b->lib.lib_OpenCnt) {
        b->lib.lib_Flags |= LIBF_DELEXP;   /* when the last one closes */
        return 0;
    }
    seg = b->seglist;
    Remove(&b->lib.lib_Node);
    rexx_freeall(b);                    /* ARexx's documents nobody freed */
    CloseLibrary((struct Library *)DOSBase);
    FreeMem((UBYTE *)b - b->lib.lib_NegSize, b->lib.lib_NegSize + b->lib.lib_PosSize);
    return seg;
}

static BPTR libclose(REG(a6, struct JsonBase *b))
{
    if (--b->lib.lib_OpenCnt == 0 && (b->lib.lib_Flags & LIBF_DELEXP))
        return libexpunge(b);
    return 0;
}

static LONG libnull(void)
{
    return 0;
}

/* no C library in here: what gcc and json.c call */
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
