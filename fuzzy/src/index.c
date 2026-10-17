/* index.c - the FuzzyFind index: writing it (updatedb), reading it, and
 * searching it.
 *
 * Format 1 (all numbers big-endian; "v" = a variable-length number, 7
 * bits a byte, low bits first, the top bit set on every byte but the last):
 *
 *   "FFIX"  UWORD 1 (format)  UWORD 0 (flags)
 *   ULONG created (seconds since 1978-01-01)  ULONG entries
 *   UWORD volumes, then for each: UBYTE length, the name, ULONG its date
 *   the entries, in the order they were added, each:
 *     v shared (bytes the path shares with the one before)
 *     v rest, then that many bytes: the path is the previous one's first
 *       "shared" bytes and these
 *     UBYTE type (FET_)  UBYTE protection  v size  v date
 *
 * updatedb adds a drawer's entries together and walks into each drawer
 * after it, so neighbours share most of their path: 21,028 paths of the
 * user's A1200 took 226 KB this way against 966 KB as a list.
 *
 * Written to NAME.new and renamed into place at the end, so a reader
 * never sees half an index. */
#include "port.h"

#define MAGIC    0x46464958UL       /* FFIX */
#define FORMAT   1
#define MAXVOL   64
#define BUFSIZE  8192

/* ---- shared ------------------------------------------------------------ */

static const char *const errtext[] = {
    "no error", "the index is not there", "not a FuzzyFind index",
    "an index format this library does not know", "out of memory",
    "reading or writing failed", "volumes must come before entries"
};

CONST_STRPTR FuzzyErrorText(LONG e)
{
    return (CONST_STRPTR)(e >= 0 && e <= FERR_ORDER ? errtext[e] : "unknown error");
}

/* the index's place: the ENV: setting FuzzyFind/Index, else S: */
LONG FuzzyIndexPath(STRPTR buf, LONG size)
{
    LONG n;
    if (!buf || size < 2) return 0;
    n = f_getvar("FuzzyFind/Index", buf, size);
    if (n > 0 && n < size) {
        while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == ' ')) buf[--n] = 0;
        if (n > 0) return n;
    }
    n = strlen(FUZZY_INDEX);
    if (n >= size) n = size - 1;
    memcpy(buf, FUZZY_INDEX, n);
    buf[n] = 0;
    return n;
}

static LONG seterr(LONG *e, LONG v)
{
    if (e) *e = v;
    return v;
}

/* ---- writing ----------------------------------------------------------- */

struct FuzzyWriter {
    FH fh;
    int started, bad;
    LONG count;
    ULONG created;
    LONG nvol;
    LONG bn;
    LONG prevlen;
    char name[MAXPATH + 8], tmp[MAXPATH + 8];
    struct { UBYTE len; UBYTE name[255]; ULONG date; } vol[MAXVOL];
    UBYTE prev[MAXPATH];
    UBYTE buf[BUFSIZE];
};

static void wflush(FuzzyWriter *w)
{
    if (w->bn && f_write(w->fh, w->buf, w->bn) != w->bn) w->bad = 1;
    w->bn = 0;
}

static void wbytes(FuzzyWriter *w, const void *s, LONG n)
{
    const UBYTE *b = s;
    while (n > 0) {
        LONG k = BUFSIZE - w->bn;
        if (k > n) k = n;
        memcpy(w->buf + w->bn, b, k);
        w->bn += k;
        b += k;
        n -= k;
        if (w->bn == BUFSIZE) wflush(w);
    }
}

static void wbyte(FuzzyWriter *w, UBYTE c) { wbytes(w, &c, 1); }

static void wlong(FuzzyWriter *w, ULONG v)
{
    UBYTE b[4];
    b[0] = (UBYTE)(v >> 24);
    b[1] = (UBYTE)(v >> 16);
    b[2] = (UBYTE)(v >> 8);
    b[3] = (UBYTE)v;
    wbytes(w, b, 4);
}

static void wword(FuzzyWriter *w, UWORD v)
{
    wbyte(w, (UBYTE)(v >> 8));
    wbyte(w, (UBYTE)v);
}

static void wvar(FuzzyWriter *w, ULONG v)
{
    while (v >= 0x80) {
        wbyte(w, (UBYTE)(v | 0x80));
        v >>= 7;
    }
    wbyte(w, (UBYTE)v);
}

static ULONG now(void)
{
#ifdef __amigaos__
    struct DateStamp ds;
    ULONG d, m;
    DateStamp(&ds);
    d = ds.ds_Days;
    m = ds.ds_Minute;
    return d * 86400UL + m * 60UL + (ULONG)ds.ds_Tick / 50;    /* once an index */
#else
    return 0;
#endif
}

FuzzyWriter *FuzzyNewIndex(CONST_STRPTR name, LONG *error)
{
    FuzzyWriter *w;
    LONG n;
    seterr(error, FERR_OK);
    if (!(w = jalloc(sizeof(*w)))) {
        seterr(error, FERR_NOMEM);
        return NULL;
    }
    w->started = w->bad = 0;
    w->count = w->nvol = w->bn = w->prevlen = 0;
    if (name) {
        n = strlen((const char *)name);
        if (n > MAXPATH) n = MAXPATH;
        memcpy(w->name, name, n);
        w->name[n] = 0;
    } else
        n = FuzzyIndexPath((STRPTR)w->name, MAXPATH);
    memcpy(w->tmp, w->name, n);
    memcpy(w->tmp + n, ".new", 5);
    if (!(w->fh = f_openw(w->tmp))) {
        seterr(error, FERR_NOFILE);
        jfree(w);
        return NULL;
    }
    w->created = now();
    return w;
}

LONG FuzzyAddVolume(FuzzyWriter *w, CONST_STRPTR name, ULONG date)
{
    LONG n;
    if (!w || !name) return FERR_IO;
    if (w->started) return FERR_ORDER;
    if (w->nvol == MAXVOL) return FERR_NOMEM;
    n = strlen((const char *)name);
    if (n > 255) n = 255;
    w->vol[w->nvol].len = (UBYTE)n;
    memcpy(w->vol[w->nvol].name, name, n);
    w->vol[w->nvol].date = date;
    w->nvol++;
    return FERR_OK;
}

static void header(FuzzyWriter *w)
{
    LONG i;
    wlong(w, MAGIC);
    wword(w, FORMAT);
    wword(w, 0);
    wlong(w, w->created);
    wlong(w, w->count);             /* again at the end */
    wword(w, (UWORD)w->nvol);
    for (i = 0; i < w->nvol; i++) {
        wbyte(w, w->vol[i].len);
        wbytes(w, w->vol[i].name, w->vol[i].len);
        wlong(w, w->vol[i].date);
    }
    w->started = 1;
}

LONG FuzzyAddEntry(FuzzyWriter *w, struct FuzzyEntry *e)
{
    const UBYTE *p;
    LONG len, k = 0;
    if (!w || !e || !e->Path) return FALSE;
    p = (const UBYTE *)e->Path;
    len = e->PathLen > 0 ? e->PathLen : (LONG)strlen((const char *)p);
    if (len > MAXPATH) return FALSE;
    if (!w->started) header(w);
    while (k < len && k < w->prevlen && p[k] == w->prev[k]) k++;
    wvar(w, k);
    wvar(w, len - k);
    wbytes(w, p + k, len - k);
    wbyte(w, e->Type);
    wbyte(w, e->Prot);
    wvar(w, e->Size);
    wvar(w, e->Date);
    memcpy(w->prev + k, p + k, len - k);
    w->prevlen = len;
    w->count++;
    return !w->bad;
}

LONG FuzzyEndIndex(FuzzyWriter *w, LONG keep)
{
    LONG rc = FERR_OK;
    UBYTE b[4];
    if (!w) return FERR_IO;
    if (keep) {
        if (!w->started) header(w);
        wflush(w);
        b[0] = (UBYTE)(w->count >> 24);
        b[1] = (UBYTE)(w->count >> 16);
        b[2] = (UBYTE)(w->count >> 8);
        b[3] = (UBYTE)w->count;
        if (!f_seek0(w->fh, 12) || f_write(w->fh, b, 4) != 4) w->bad = 1;
    }
    f_close(w->fh);
    if (!keep || w->bad) {
        f_delete(w->tmp);
        rc = keep ? FERR_IO : FERR_OK;
    } else {
        f_delete(w->name);          /* Rename will not replace */
        if (!f_rename(w->tmp, w->name)) rc = FERR_IO;
    }
    jfree(w);
    return rc;
}

/* ---- reading ----------------------------------------------------------- */

struct Index {
    struct FuzzyIndex pub;
    FH fh;
    LONG start;                     /* where the entries begin */
    LONG pos;                       /* the file position of buf[0] + blen */
    LONG bp, blen;
    LONG read;
    LONG pathlen;
    LONG nameoff;                   /* where the name starts in path */
    int bad, eof;
    struct { UBYTE len; UBYTE name[256]; ULONG date; } vol[MAXVOL];
    ULONG mtab[256];                /* a byte's bit in the letter fingerprint */
    ULONG mask[MAXPATH + 1];        /* the fingerprint of path[0..i) */
    UBYTE path[MAXPATH + 1];
    UBYTE buf[BUFSIZE];
};

#define IX(x) ((struct Index *)(x))

/* The letter fingerprint: which letters a path has, as 32 bits (a-z,
 * digits, . - _ space, the rest), case and accents folded away. A search
 * skips a path whose fingerprint lacks one of the pattern's letters with
 * one AND, without looking at its letters: on a 68000 that is what makes
 * searching thousands of paths bearable. Kept per position while the
 * entries are read (only the bytes new to each entry), so the index file
 * does not carry it. */
static void mkmtab(ULONG *t)
{
    static const char un[] = "aaaaaaaceeeeiiiidnooooo.ouuuuyts" "aaaaaaaceeeeiiiidnooooo.ouuuuyty";
    int c;
    for (c = 0; c < 256; c++) {
        int f = c;
        if (f >= 0xC0 && un[f - 0xC0] != '.') f = un[f - 0xC0];
        if (f >= 'A' && f <= 'Z') f += 32;
        if (f >= 'a' && f <= 'z') t[c] = 1UL << (f - 'a');
        else if (f >= '0' && f <= '9') t[c] = 1UL << 26;
        else if (f == '.') t[c] = 1UL << 27;
        else if (f == '-' || f == '_') t[c] = 1UL << 28;
        else if (f == ' ') t[c] = 1UL << 29;
        else if (f == '/' || f == ':') t[c] = 1UL << 31;
        else t[c] = 1UL << 30;
    }
}

static int rfill(struct Index *x)
{
    LONG n = f_read(x->fh, x->buf, BUFSIZE);
    if (n <= 0) {
        x->bad = 1;
        return 0;
    }
    x->bp = 0;
    x->blen = n;
    x->pos += n;
    return 1;
}

static int rbyte(struct Index *x, UBYTE *c)
{
    if (x->bp >= x->blen && !rfill(x)) return 0;
    *c = x->buf[x->bp++];
    return 1;
}

static int rbytes(struct Index *x, UBYTE *d, LONG n)
{
    while (n > 0) {
        LONG k;
        if (x->bp >= x->blen && !rfill(x)) return 0;
        k = x->blen - x->bp;
        if (k > n) k = n;
        memcpy(d, x->buf + x->bp, k);
        x->bp += k;
        d += k;
        n -= k;
    }
    return 1;
}

static int rlong(struct Index *x, ULONG *v)
{
    UBYTE b[4];
    if (!rbytes(x, b, 4)) return 0;
    *v = ((ULONG)b[0] << 24) | ((ULONG)b[1] << 16) | ((ULONG)b[2] << 8) | b[3];
    return 1;
}

static int rword(struct Index *x, UWORD *v)
{
    UBYTE b[2];
    if (!rbytes(x, b, 2)) return 0;
    *v = (UWORD)((b[0] << 8) | b[1]);
    return 1;
}

static int rvar(struct Index *x, ULONG *v) __attribute__((unused));
static int rvar(struct Index *x, ULONG *v)
{
    ULONG r = 0;
    int sh = 0;
    UBYTE c;
    do {
        if (!rbyte(x, &c) || sh > 28) return 0;
        r |= (ULONG)(c & 0x7F) << sh;
        sh += 7;
    } while (c & 0x80);
    *v = r;
    return 1;
}

struct FuzzyIndex *FuzzyOpenIndex(CONST_STRPTR name, LONG *error)
{
    struct Index *x;
    char path[MAXPATH + 8];
    ULONG magic, created, count;
    UWORD format, flags, nvol, i;
    seterr(error, FERR_OK);
    if (!name) {
        FuzzyIndexPath((STRPTR)path, MAXPATH);
        name = (CONST_STRPTR)path;
    }
    if (!(x = jalloc(sizeof(*x)))) {
        seterr(error, FERR_NOMEM);
        return NULL;
    }
    x->bp = x->blen = x->pos = 0;
    x->read = x->pathlen = x->nameoff = 0;
    x->bad = x->eof = 0;
    if (!(x->fh = f_openr(name))) {
        jfree(x);
        seterr(error, FERR_NOFILE);
        return NULL;
    }
    if (!rlong(x, &magic) || magic != MAGIC) {
        seterr(error, FERR_NOTINDEX);
        goto fail;
    }
    if (!rword(x, &format) || format != FORMAT) {
        seterr(error, FERR_VERSION);
        goto fail;
    }
    if (!rword(x, &flags) || !rlong(x, &created) || !rlong(x, &count) || !rword(x, &nvol) ||
        nvol > MAXVOL || (LONG)count < 0) {
        seterr(error, FERR_NOTINDEX);
        goto fail;
    }
    for (i = 0; i < nvol; i++) {
        if (!rbyte(x, &x->vol[i].len) || !rbytes(x, x->vol[i].name, x->vol[i].len) ||
            !rlong(x, &x->vol[i].date)) {
            seterr(error, FERR_NOTINDEX);
            goto fail;
        }
        x->vol[i].name[x->vol[i].len] = 0;
    }
    mkmtab(x->mtab);
    x->mask[0] = 0;
    x->pub.Count = (LONG)count;
    x->pub.Created = created;
    x->pub.Volumes = nvol;
    x->start = x->pos - x->blen + x->bp;
    return &x->pub;
fail:
    f_close(x->fh);
    jfree(x);
    return NULL;
}

void FuzzyCloseIndex(struct FuzzyIndex *p)
{
    struct Index *x = IX(p);
    if (!x) return;
    f_close(x->fh);
    jfree(x);
}

CONST_STRPTR FuzzyVolume(struct FuzzyIndex *p, LONG n, ULONG *date)
{
    struct Index *x = IX(p);
    if (!x || n < 0 || n >= x->pub.Volumes) return NULL;
    if (date) *date = x->vol[n].date;
    return (CONST_STRPTR)x->vol[n].name;
}

void FuzzyRewind(struct FuzzyIndex *p)
{
    struct Index *x = IX(p);
    if (!x) return;
    x->read = x->pathlen = x->nameoff = 0;
    x->bad = 0;
    if (x->start >= x->pos - x->blen && x->start <= x->pos)
        x->bp = x->start - (x->pos - x->blen);  /* still in the buffer */
    else {
        x->bp = x->blen = 0;
        x->pos = x->start;
        x->eof = 0;
        if (!f_seek0(x->fh, x->start)) x->bad = 1;
    }
}

/* the buffer topped up so that a whole entry is in it (a path's rest is at
 * most MAXPATH, the numbers at most 4 x 5 bytes, the two bytes): then an
 * entry is read straight from memory, without a call a byte - on a 68000
 * that alone took a 21,000-entry index from 21 s to a fraction */
#define ENTRYMAX (MAXPATH + 24)

static void topup(struct Index *x)
{
    LONG left = x->blen - x->bp, n;
    UBYTE *d, *q;
    if (left >= ENTRYMAX || x->eof) return;
    for (d = x->buf, q = x->buf + x->bp, n = left; n > 0; n--) *d++ = *q++;   /* down: safe */
    x->bp = 0;
    x->blen = left;
    n = f_read(x->fh, x->buf + left, BUFSIZE - left);
    if (n > 0) {
        x->blen += n;
        x->pos += n;
    } else
        x->eof = 1;
}

/* a variable-length number at *q (inside the buffer) */
#define GETVAR(v) do { \
        ULONG r_ = 0; int sh_ = 0; UBYTE c_; \
        do { if (q >= end || sh_ > 28) goto bad; c_ = *q++; r_ |= (ULONG)(c_ & 0x7F) << sh_; sh_ += 7; } \
        while (c_ & 0x80); \
        (v) = r_; \
    } while (0)

/* the next entry into x->path (and its fingerprint into x->mask[]), its
 * type, protection, size and date into the arguments */
static inline int nextraw(struct Index *x, UBYTE *type, UBYTE *prot, ULONG *size, ULONG *date, int domask)
{
    ULONG shared, rest, m;
    const UBYTE *q, *end;
    UBYTE *d;
    ULONG *mk;
    const ULONG *mt = x->mtab;
    LONG n;
    if (x->bad || x->read >= x->pub.Count) return 0;
    topup(x);
    q = x->buf + x->bp;
    end = x->buf + x->blen;
    GETVAR(shared);
    GETVAR(rest);
    if (shared > (ULONG)x->pathlen || rest > (ULONG)(MAXPATH - shared) || q + rest + 2 > end) goto bad;
    d = x->path + shared;
    if (domask) {                       /* FuzzySearch: it rewinds first, so all are made */
        mk = x->mask + shared;
        m = *mk++;
        fz_copymask(q, d, mk, mt, rest, m);     /* the new bytes, and their fingerprint */
        q += rest;
        d += rest;
    } else {
        for (n = rest; n > 0; n--) *d++ = *q++;
    }
    *type = *q++;
    *prot = *q++;
    GETVAR(*size);
    GETVAR(*date);
    x->bp = q - x->buf;
    x->pathlen = shared + rest;
    *d = 0;
    x->read++;
    /* where the name starts, without scanning the whole path: after a / or
     * : in the new bytes; else where the last path's name started, if the
     * shared part reaches it (no separator can be between); else scan the
     * shared part (going up out of a drawer). On a 68000 scanning every
     * path for its name was a quarter of reading the index. */
    {
        const UBYTE *pp = x->path;
        LONG i = x->pathlen;
        while (i > (LONG)shared && pp[i - 1] != '/' && pp[i - 1] != ':') i--;
        if (i == (LONG)shared && (LONG)shared < x->nameoff)
            while (i > 0 && pp[i - 1] != '/' && pp[i - 1] != ':') i--;
        else if (i == (LONG)shared && i > 0 && pp[i - 1] != '/' && pp[i - 1] != ':')
            i = x->nameoff;
        x->nameoff = i;
    }
    return 1;
bad:
    x->bad = 1;                     /* cut short or damaged: the end */
    return 0;
}

LONG FuzzyNextEntry(struct FuzzyIndex *p, struct FuzzyEntry *e)
{
    struct Index *x = IX(p);
    if (!x || !e || !nextraw(x, &e->Type, &e->Prot, &e->Size, &e->Date, 0)) return FALSE;
    e->Path = (CONST_STRPTR)x->path;
    e->PathLen = x->pathlen;
    e->NameOffset = x->nameoff;
    e->Pad = 0;
    return TRUE;
}

/* ---- searching --------------------------------------------------------- */

/* a is better than b: higher score, then the shorter path, then earlier */
static int better(const struct FuzzyHit *a, LONG ai, const struct FuzzyHit *b, LONG bi)
{
    if (a->Score != b->Score) return a->Score > b->Score;
    if (a->PathLen != b->PathLen) return a->PathLen < b->PathLen;
    return ai < bi;
}

struct Results {
    struct FuzzyResults pub;
    LONG *order;                    /* each hit's entry number (ties) */
};

struct FuzzyResults *FuzzySearch(struct FuzzyIndex *p, CONST_STRPTR pattern, ULONG flags, LONG max)
{
    struct Index *x = IX(p);
    struct Results *r;
    FuzzyPattern *pat;
    struct FuzzyEntry e;
    LONG n = 0, entry = 0;
    ULONG pmask;
    if (!x || max <= 0) return NULL;
    if (!(pat = FuzzyPrepare(pattern ? pattern : (CONST_STRPTR)"", flags & 0xFF))) return NULL;
    if (!(r = jalloc(sizeof(*r)))) {
        FuzzyFreePattern(pat);
        return NULL;
    }
    r->pub.Count = r->pub.Matched = 0;
    r->pub.Hits = jalloc(max * sizeof(struct FuzzyHit));
    r->order = jalloc(max * sizeof(LONG));
    if (!r->pub.Hits || !r->order) {
        if (r->pub.Hits) jfree(r->pub.Hits);
        if (r->order) jfree(r->order);
        jfree(r);
        FuzzyFreePattern(pat);
        return NULL;
    }
    pmask = fuzzy_patmask(pat, x->mtab);
    FuzzyRewind(p);
    for (; nextraw(x, &e.Type, &e.Prot, &e.Size, &e.Date, 1); entry++) {
        struct FuzzyHit h;
        LONG sc, i;
        UBYTE *copy;
        if (pmask & ~x->mask[x->pathlen]) continue;     /* a letter it lacks */
#ifdef PROF
        { extern LONG prof_masked; prof_masked++; }
#endif
        if ((flags & FSF_FILES) && e.Type != FET_FILE) continue;
        if ((flags & FSF_DRAWERS) && e.Type != FET_DRAWER) continue;
        e.Path = (CONST_STRPTR)x->path;
        e.PathLen = x->pathlen;
        e.NameOffset = x->nameoff;
        if (flags & FSF_NAME)
            sc = fuzzy_score(pat, (const UBYTE *)e.Path + e.NameOffset, e.PathLen - e.NameOffset, 0, NULL, 0);
        else
            sc = fuzzy_score(pat, (const UBYTE *)e.Path, e.PathLen, e.NameOffset, NULL, 0);
        if (sc <= 0) continue;
        r->pub.Matched++;
        h.Score = sc;
        h.PathLen = e.PathLen;
        if (n == max && !better(&h, entry, &r->pub.Hits[n - 1], r->order[n - 1])) continue;
        if (!(copy = jalloc(e.PathLen + 1))) continue;
        memcpy(copy, e.Path, e.PathLen + 1);
        h.Path = (CONST_STRPTR)copy;
        h.NameOffset = e.NameOffset;
        h.Type = e.Type;
        h.Prot = e.Prot;
        h.Pad = 0;
        h.Size = e.Size;
        h.Date = e.Date;
        if (n == max) jfree((APTR)r->pub.Hits[--n].Path);   /* the worst goes */
        for (i = n; i > 0 && better(&h, entry, &r->pub.Hits[i - 1], r->order[i - 1]); i--) {
            r->pub.Hits[i] = r->pub.Hits[i - 1];
            r->order[i] = r->order[i - 1];
        }
        r->pub.Hits[i] = h;
        r->order[i] = entry;
        n++;
    }
    r->pub.Count = n;
    FuzzyFreePattern(pat);
    return &r->pub;
}

void FuzzyFreeResults(struct FuzzyResults *p)
{
    struct Results *r = (struct Results *)p;
    LONG i;
    if (!r) return;
    for (i = 0; i < r->pub.Count; i++) jfree((APTR)r->pub.Hits[i].Path);
    jfree(r->pub.Hits);
    jfree(r->order);
    jfree(r);
}
