/* rexx.c - json.library as an ARexx function library.
 *
 *   CALL ADDLIB('json.library', 0, -30, 0)
 *
 * ARexx calls the library's first entry (offset -30, JsonRexxEntry in
 * start.s) with a RexxMsg for every function it does not know; the
 * function's name is ARG0. A name that is not ours gets error 1, so
 * ARexx asks the next library.
 *
 * Documents are handles ("JSON1", "JSON2"...) in a list in the library
 * base, shared by all scripts and freed with JSONFREE (or when the
 * library leaves memory). ARexx text is Latin-1: it is turned into UTF-8
 * going in and back to Latin-1 coming out; JSON text for other programs
 * (JSONTEXT, JSONSAVE) stays UTF-8. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <rexx/storage.h>
#include <rexx/errors.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>
#include <stddef.h>
#include "port.h"
#include "base.h"

struct RxsLib *RexxSysBase;

#ifdef DBG
/* a trace line through exec's RawPutChar (LVO -516), for wasabi debug */
static void dput(char c)
{
    register char d0 __asm("d0") = c;
    register struct ExecBase *a6 __asm("a6") = SysBase;
    __asm volatile ("jsr -516(a6)" : "+r"(d0) : "r"(a6) : "d1", "a0", "a1", "cc", "memory");
}
static void dbg(const char *s, ULONG v)
{
    int i;
    while (*s) dput(*s++);
    dput(' ');
    for (i = 28; i >= 0; i -= 4) dput("0123456789abcdef"[(v >> i) & 15]);
    dput('\n');
}
#else
#define dbg(s, v)
#endif

struct Handle {
    struct MinNode node;
    ULONG id;
    JsonDoc *doc;
};

#define ALEN(a) (((struct RexxArg *)((UBYTE *)(a) - offsetof(struct RexxArg, ra_Buff)))->ra_Length)
#define MAXREXX 65535               /* the longest ARexx string */

/* ---- small helpers ----------------------------------------------------- */

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

static void seterr(struct JsonBase *b, const char *msg, struct JsonError *e)
{
    char *o = b->lasterr;
    int k = 0;
    if (e) {
        k += itoa(e->Line, o + k);
        o[k++] = ':';
        k += itoa(e->Col, o + k);
        o[k++] = ' ';
        msg = e->Message ? (const char *)e->Message : "out of memory";
    }
    while (*msg && k < (int)sizeof(b->lasterr) - 1) o[k++] = *msg++;
    o[k] = 0;
}

static LONG result(UBYTE **res, const char *s, LONG len)
{
    if (len < 0) len = strlen(s);
    if (len > MAXREXX) len = MAXREXX;
    *res = CreateArgstring((CONST_STRPTR)s, len);
    dbg("json: result", (ULONG)*res);
    return *res ? 0 : ERR10_003;
}

/* ---- handles ----------------------------------------------------------- */

static LONG newhandle(struct JsonBase *b, JsonDoc *d, UBYTE **res)
{
    struct Handle *h = AllocVec(sizeof(*h), MEMF_ANY);
    char name[16];
    if (!h) {
        JsonFree(d);
        return ERR10_003;
    }
    h->doc = d;
    ObtainSemaphore(&b->lock);
    h->id = ++b->nextid;
    AddTail((struct List *)&b->handles, (struct Node *)h);
    ReleaseSemaphore(&b->lock);
    memcpy(name, "JSON", 4);
    itoa(h->id, name + 4);
    return result(res, name, -1);
}

static struct Handle *findhandle(struct JsonBase *b, const UBYTE *s)
{
    struct Handle *h;
    ULONG id = 0;
    if (!s || upper(s[0]) != 'J' || upper(s[1]) != 'S' || upper(s[2]) != 'O' || upper(s[3]) != 'N')
        return NULL;
    for (s += 4; *s >= '0' && *s <= '9'; s++) id = (id << 3) + (id << 1) + (*s - '0');
    if (*s) return NULL;
    ObtainSemaphore(&b->lock);
    for (h = (struct Handle *)b->handles.mlh_Head; h->node.mln_Succ; h = (struct Handle *)h->node.mln_Succ)
        if (h->id == id) break;
    ReleaseSemaphore(&b->lock);
    return h->node.mln_Succ ? h : NULL;
}

void rexx_freeall(struct JsonBase *b)
{
    struct Handle *h;
    while ((h = (struct Handle *)RemHead((struct List *)&b->handles)) != NULL) {
        JsonFree(h->doc);
        FreeVec(h);
    }
    if (RexxSysBase) {
        CloseLibrary((struct Library *)RexxSysBase);
        RexxSysBase = NULL;
    }
}

/* the node at an ARexx path (Latin-1, turned into UTF-8 first) */
static JsonNode *at(JsonDoc *d, const UBYTE *path)
{
    UBYTE *u;
    JsonNode *n;
    if (!path || !*path) return JsonRoot(d);
    if (!(u = json_latin1_to_u8(path, strlen((const char *)path), NULL))) return NULL;
    n = JsonFind(JsonRoot(d), (CONST_STRPTR)u);
    FreeVec(u);
    return n;
}

/* ---- JSONSET: the path made where it is missing ------------------------ */

/* the next part of a path at *pp: a name (into name, UTF-8, 0-ended) or
 * an index; 0 at the end, -1 when the path is wrong */
static int part(const UBYTE **pp, UBYTE *name, LONG size, LONG *index)
{
    const UBYTE *s = *pp;
    LONG k = 0;
    while (*s == '.') s++;
    if (!*s) return 0;
    if (*s == '[') {
        s++;
        if (*s == '"') {
            for (s++; *s && *s != '"'; s++) {
                if (*s == '\\' && s[1]) s++;
                if (k < size - 1) name[k++] = *s;
            }
            if (*s != '"' || s[1] != ']') return -1;
            name[k] = 0;
            *pp = s + 2;
            return 1;
        }
        *index = 0;
        if (*s < '0' || *s > '9') return -1;
        while (*s >= '0' && *s <= '9') {
            if (*index < 100000000) *index = (*index << 3) + (*index << 1) + (*s - '0');
            s++;
        }
        if (*s++ != ']') return -1;
        *pp = s;
        return 2;
    }
    while (*s && *s != '.' && *s != '[') {
        if (k < size - 1) name[k++] = *s;
        s++;
    }
    name[k] = 0;
    *pp = s;
    return 1;
}

/* put n at path in d, making objects (or arrays, before an [index]) on
 * the way; an [index] one past the end adds to the array */
static int setat(JsonDoc *d, const UBYTE *path, JsonNode *n)
{
    UBYTE name[256], next[256];
    LONG index, nindex;
    const UBYTE *p;
    JsonNode *cur = JsonRoot(d), *c;
    int kind, nk;
    UBYTE *u;
    LONG ul;
    if (!path || !*path) {
        JsonSetRoot(d, n);
        return 1;
    }
    if (!(u = json_latin1_to_u8(path, strlen((const char *)path), &ul))) return 0;
    p = u;
    kind = part(&p, name, sizeof(name), &index);
    if (kind <= 0) {
        FreeVec(u);
        return 0;
    }
    if (!cur) {
        if (!(cur = kind == 2 ? JsonNewArray(d) : JsonNewObject(d))) goto bad;
        JsonSetRoot(d, cur);
    }
    for (;;) {
        nk = part(&p, next, sizeof(next), &nindex);
        if (nk < 0) goto bad;
        if (kind == 1 && JsonType(cur) != JT_OBJECT) goto bad;
        if (kind == 2 && JsonType(cur) != JT_ARRAY) goto bad;
        c = kind == 1 ? JsonGet(cur, (CONST_STRPTR)name) : JsonAt(cur, index);
        if (kind == 2 && !c && index != JsonCount(cur)) goto bad;  /* no holes */
        if (nk == 0) {                  /* the last part: n goes here */
            if (c) json_replace(c, n);
            else if (!JsonAdd(cur, (CONST_STRPTR)name, n)) goto bad;
            FreeVec(u);
            return 1;
        }
        if (!c) {
            if (!(c = nk == 2 ? JsonNewArray(d) : JsonNewObject(d))) goto bad;
            if (!JsonAdd(cur, (CONST_STRPTR)name, c)) goto bad;
        }
        cur = c;
        kind = nk;
        memcpy(name, next, sizeof(name));
        index = nindex;
    }
bad:
    FreeVec(u);
    return 0;
}

/* ---- the functions ----------------------------------------------------- */

/* JSONGET's value as ARexx text, into a new string */
static LONG value(struct JsonBase *b, JsonNode *n, const UBYTE *def, UBYTE **res)
{
    char *t;
    LONG len, rc;
    switch (JsonType(n)) {
    case JT_NONE:
        return result(res, def ? (const char *)def : "", def ? ALEN(def) : 0);
    case JT_NULL:
        return result(res, "", 0);
    case JT_BOOL:
        return result(res, JsonBool(n, 0) ? "1" : "0", 1);
    case JT_STRING:
    case JT_NUMBER:
        len = JsonLatin1(n, NULL, 0);
        if (len > MAXREXX) break;
        if (!(t = AllocVec(len + 1, MEMF_ANY))) return ERR10_003;
        JsonLatin1(n, t, len + 1);
        rc = result(res, t, len);
        FreeVec(t);
        return rc;
    default:
        /* an object or array: as JSON, ASCII only so it survives as Latin-1 */
        len = JsonWrite(n, NULL, 0, JWF_ASCII);
        if (len > MAXREXX) break;
        if (!(t = AllocVec(len + 1, MEMF_ANY))) return ERR10_003;
        JsonWrite(n, t, len + 1, JWF_ASCII);
        rc = result(res, t, len);
        FreeVec(t);
        return rc;
    }
    seterr(b, "too long for an ARexx string: use JSONSAVE", NULL);
    return result(res, "", 0);
}

/* the node JSONSET puts in: its type from the 4th argument, else a
 * number if the value is one and text otherwise */
static JsonNode *makenode(struct JsonBase *b, JsonDoc *d, const UBYTE *v, const UBYTE *type, LONG *rc)
{
    LONG vl = v ? ALEN(v) : 0;
    struct JsonError e;
    JsonNode *n = NULL;
    *rc = ERR10_018;
    seterr(b, "the value is not of that type", NULL);
    if (!v) v = (const UBYTE *)"";
    if (!type || !*type)
        n = json_isnumber(v, vl) ? JsonNewNumber(d, (CONST_STRPTR)v) : JsonNewString(d, (CONST_STRPTR)v, vl, JSF_LATIN1);
    else if (same(type, "STRING"))
        n = JsonNewString(d, (CONST_STRPTR)v, vl, JSF_LATIN1);
    else if (same(type, "NUMBER")) {
        if (!json_isnumber(v, vl)) return NULL;
        n = JsonNewNumber(d, (CONST_STRPTR)v);
    } else if (same(type, "BOOL")) {
        if (same(v, "1") || same(v, "TRUE") || same(v, "YES") || same(v, "ON")) n = JsonNewBool(d, 1);
        else if (same(v, "0") || same(v, "FALSE") || same(v, "NO") || same(v, "OFF") || !*v) n = JsonNewBool(d, 0);
        else return NULL;
    } else if (same(type, "NULL"))
        n = JsonNewNull(d);
    else if (same(type, "OBJECT"))
        n = JsonNewObject(d);
    else if (same(type, "ARRAY"))
        n = JsonNewArray(d);
    else if (same(type, "JSON")) {
        if (!(n = json_parse_into(d, v, vl, JPF_RELAXED, &e))) {
            seterr(b, NULL, &e);
            return NULL;
        }
    } else
        return NULL;
    if (!n) *rc = ERR10_003;
    else *rc = 0;
    return n;
}

static LONG text(struct JsonBase *b, struct Handle *h, const UBYTE *path, const UBYTE *opts, UBYTE **res)
{
    JsonNode *n = at(h->doc, path);
    ULONG fl = (hasopt(opts, "PRETTY") ? JWF_PRETTY : 0) | (hasopt(opts, "ASCII") ? JWF_ASCII : 0);
    LONG len, rc;
    char *t;
    if (!n) return result(res, "", 0);
    len = JsonWrite(n, NULL, 0, fl);
    if (len > MAXREXX) {
        seterr(b, "too long for an ARexx string: use JSONSAVE", NULL);
        return result(res, "", 0);
    }
    if (!(t = AllocVec(len + 1, MEMF_ANY))) return ERR10_003;
    JsonWrite(n, t, len + 1, fl);
    rc = result(res, t, len);
    FreeVec(t);
    return rc;
}

/* called from JsonRexxEntry (start.s): an ARexx error code back, and the
 * result string in *res */
LONG rexx_query(struct JsonBase *b, struct RexxMsg *m, UBYTE **res)
{
    const UBYTE *fn = (const UBYTE *)m->rm_Args[0], *a1, *a2, *a3, *a4;
    int nargs = m->rm_Action & RXARGMASK;
    struct Handle *h;
    struct JsonError e;
    JsonDoc *d;
    JsonNode *n;
    LONG rc;
    char num[12];

    dbg("json: query msg", (ULONG)m);
    dbg("json: base", (ULONG)b);
    dbg("json: fn", (ULONG)fn);
    *res = NULL;
    if (!fn || (upper(fn[0]) != 'J' || upper(fn[1]) != 'S' || upper(fn[2]) != 'O' || upper(fn[3]) != 'N'))
        return ERR10_001;           /* not ours: the next library */
    if (!RexxSysBase) {
        ObtainSemaphore(&b->lock);
        if (!RexxSysBase) RexxSysBase = (struct RxsLib *)OpenLibrary("rexxsyslib.library", 36);
        ReleaseSemaphore(&b->lock);
        if (!RexxSysBase) return ERR10_003;
    }
    dbg("json: rexxsys", (ULONG)RexxSysBase);
    a1 = nargs >= 1 ? (const UBYTE *)m->rm_Args[1] : NULL;
    a2 = nargs >= 2 ? (const UBYTE *)m->rm_Args[2] : NULL;
    a3 = nargs >= 3 ? (const UBYTE *)m->rm_Args[3] : NULL;
    a4 = nargs >= 4 ? (const UBYTE *)m->rm_Args[4] : NULL;
    fn += 4;

    if (same(fn, "PARSE")) {                /* JSONPARSE(text, [options]) */
        if (!a1) return ERR10_017;
        if (!(d = JsonParse((CONST_STRPTR)a1, ALEN(a1), (hasopt(a2, "RELAXED") ? JPF_RELAXED : 0) | JPF_LATIN1, &e))) {
            seterr(b, NULL, &e);
            return result(res, "", 0);
        }
        return newhandle(b, d, res);
    }
    if (same(fn, "LOAD")) {                 /* JSONLOAD(file, [options]) */
        if (!a1) return ERR10_017;
        if (!(d = JsonLoad((CONST_STRPTR)a1, hasopt(a2, "RELAXED") ? JPF_RELAXED : 0, &e))) {
            seterr(b, NULL, &e);
            return result(res, "", 0);
        }
        return newhandle(b, d, res);
    }
    if (same(fn, "NEW")) {                  /* JSONNEW([ARRAY]) */
        if (!(d = JsonNewDoc())) return ERR10_003;
        n = hasopt(a1, "ARRAY") ? JsonNewArray(d) : JsonNewObject(d);
        if (!n) {
            JsonFree(d);
            return ERR10_003;
        }
        JsonSetRoot(d, n);
        return newhandle(b, d, res);
    }
    if (same(fn, "ERROR"))                  /* JSONERROR() */
        return result(res, b->lasterr, -1);

    /* the rest take a handle first */
    if (!a1) return ERR10_017;
    if (!(h = findhandle(b, a1))) return ERR10_018;

    if (same(fn, "FREE")) {                 /* JSONFREE(handle) */
        ObtainSemaphore(&b->lock);
        Remove((struct Node *)h);
        ReleaseSemaphore(&b->lock);
        JsonFree(h->doc);
        FreeVec(h);
        return result(res, "1", 1);
    }
    if (same(fn, "GET"))                    /* JSONGET(handle, path, [default]) */
        return value(b, at(h->doc, a2), a3, res);
    if (same(fn, "TYPE")) {                 /* JSONTYPE(handle, path) */
        static const char *const names[] = { "", "NULL", "BOOL", "NUMBER", "STRING", "ARRAY", "OBJECT" };
        return result(res, names[JsonType(at(h->doc, a2))], -1);
    }
    if (same(fn, "COUNT"))                  /* JSONCOUNT(handle, path) */
        return result(res, num, itoa(JsonCount(at(h->doc, a2)), num));
    if (same(fn, "NAME")) {                 /* JSONNAME(handle, path, n) */
        LONG i = 0;
        const UBYTE *s = a3;
        char *o;
        if (!s) return ERR10_017;
        for (; *s >= '0' && *s <= '9'; s++) i = (i << 3) + (i << 1) + (*s - '0');
        n = JsonFirst(at(h->doc, a2));
        while (n && i--) n = JsonNext(n);
        if (!n || !JsonName(n)) return result(res, "", 0);
        rc = json_tolatin1((const UBYTE *)JsonName(n), strlen((const char *)JsonName(n)), NULL, 0);
        if (!(o = AllocVec(rc + 1, MEMF_ANY))) return ERR10_003;
        json_tolatin1((const UBYTE *)JsonName(n), strlen((const char *)JsonName(n)), o, rc + 1);
        rc = result(res, o, rc);
        FreeVec(o);
        return rc;
    }
    if (same(fn, "SET")) {                  /* JSONSET(handle, path, value, [type]) */
        /* what cannot be done is 0 and JSONERROR() says why, as for
         * JSONPARSE: an error code would stop the script */
        n = makenode(b, h->doc, a3, a4, &rc);
        if (!n) {
            if (rc == ERR10_003) return rc;
            return result(res, "0", 1);
        }
        if (!setat(h->doc, a2, n)) {
            seterr(b, "the path cannot be set there", NULL);
            return result(res, "0", 1);
        }
        return result(res, "1", 1);
    }
    if (same(fn, "REMOVE")) {               /* JSONREMOVE(handle, path) */
        if (!a2 || !*a2 || !(n = at(h->doc, a2))) return result(res, "0", 1);
        JsonRemove(n);
        return result(res, "1", 1);
    }
    if (same(fn, "TEXT"))                   /* JSONTEXT(handle, [path], [options]) */
        return text(b, h, a2, a3, res);
    if (same(fn, "SAVE")) {                 /* JSONSAVE(handle, file, [options]) */
        BPTR fh;
        LONG ok;
        if (!a2) return ERR10_017;
        if (!(fh = Open((STRPTR)a2, MODE_NEWFILE))) {
            seterr(b, "cannot open the file", NULL);
            return result(res, "0", 1);
        }
        ok = JsonWriteFH(JsonRoot(h->doc), fh, (hasopt(a3, "PRETTY") ? JWF_PRETTY : 0) |
                                               (hasopt(a3, "ASCII") ? JWF_ASCII : 0));
        if (!Close(fh)) ok = 0;
        if (!ok) seterr(b, "cannot write the file", NULL);
        return result(res, ok ? "1" : "0", 1);
    }
    return ERR10_001;                       /* JSON-something we do not have */
}
