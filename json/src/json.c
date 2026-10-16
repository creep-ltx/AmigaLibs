/* json.c - json.library's core: reading, walking, building and writing
 * JSON. Plain C over port.h, so the same file is the library on the
 * Amiga and the code under test on the PC.
 *
 * Built for small machines:
 *  - no recursion: reading, writing and freeing walk the tree with the
 *    parent links, so a deeply nested file cannot overflow a 4 KB stack;
 *  - no floating point, and no multiply or divide in the loops (a 68000
 *    has no 32-bit ones): numbers keep their text;
 *  - one memory pool a document: JsonFree() gives back a few big blocks,
 *    not every node. */
#include "port.h"

/* ---- memory ------------------------------------------------------------ */

struct Block {
    struct Block *next;
    ULONG size, used;               /* the data after this header */
};

struct JsonDoc {
    struct Block *blocks;
    JsonNode *root;
    ULONG grow;                     /* the next block's size */
};

struct JsonNode {
    JsonNode *next, *parent;
    JsonNode *first, *last;         /* an array's or object's members */
    JsonDoc *doc;
    const UBYTE *name;              /* an object member's name (UTF-8) */
    const UBYTE *str;               /* a string, or a number's text */
    LONG namelen, len;
    LONG count;                     /* members */
    LONG ival;                      /* a number that is a whole LONG; a bool */
    UBYTE type;
    UBYTE intok;                    /* ival is the number exactly */
};

#define FIRSTBLOCK 2048
#define MAXBLOCK   65536

static void *palloc(JsonDoc *d, ULONG n)
{
    struct Block *b = d->blocks, *nb;
    n = (n + sizeof(APTR) - 1) & ~(ULONG)(sizeof(APTR) - 1);   /* 4 on the Amiga */
    if (!b || b->size - b->used < n) {
        ULONG sz = d->grow;
        if (n > sz / 4) {           /* a big string: a block of its own, behind */
            if (!(nb = jalloc(sizeof(*nb) + n))) return NULL;
            nb->size = nb->used = n;
            if (b) {
                nb->next = b->next;
                b->next = nb;
            } else {
                nb->next = NULL;
                d->blocks = nb;
            }
            return nb + 1;
        }
        if (!(nb = jalloc(sizeof(*nb) + sz))) return NULL;
        nb->size = sz;
        nb->used = 0;
        nb->next = b;
        d->blocks = b = nb;
        if (d->grow < MAXBLOCK) d->grow <<= 1;
    }
    b->used += n;
    return (UBYTE *)(b + 1) + b->used - n;
}

static JsonNode *newnode(JsonDoc *d, int type)
{
    JsonNode *n;
    if (!d || !(n = palloc(d, sizeof(*n)))) return NULL;
    memset(n, 0, sizeof(*n));
    n->doc = d;
    n->type = (UBYTE)type;
    return n;
}

JsonDoc *JsonNewDoc(void)
{
    JsonDoc *d = jalloc(sizeof(*d));
    if (d) {
        d->blocks = NULL;
        d->root = NULL;
        d->grow = FIRSTBLOCK;
    }
    return d;
}

void JsonFree(JsonDoc *d)
{
    struct Block *b, *nb;
    if (!d) return;
    for (b = d->blocks; b; b = nb) {
        nb = b->next;
        jfree(b);
    }
    jfree(d);
}

/* ---- text -------------------------------------------------------------- */

/* the length of a whole UTF-8 sequence of 2-4 bytes at q (strict: no
 * overlong forms, no surrogates, nothing past U+10FFFF), or 0 */
static int u8seq(const UBYTE *q, const UBYTE *e)
{
    UBYTE c = q[0];
    int k, i;
    if (c < 0xC2 || c > 0xF4) return 0;
    k = c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
    if (e - q <= k) return 0;
    for (i = 1; i <= k; i++)
        if ((q[i] & 0xC0) != 0x80) return 0;
    if ((c == 0xE0 && q[1] < 0xA0) || (c == 0xED && q[1] >= 0xA0) ||
        (c == 0xF0 && q[1] < 0x90) || (c == 0xF4 && q[1] >= 0x90))
        return 0;
    return k + 1;
}

static ULONG u8decode(const UBYTE *q, int k)
{
    ULONG u = q[0] & (0x7F >> k);
    int i;
    for (i = 1; i < k; i++) u = (u << 6) | (q[i] & 0x3F);
    return u;
}

/* a code point as UTF-8 into o (NULL: only counted); its length */
static int u8put(UBYTE *o, ULONG u)
{
    if (u < 0x80) {
        if (o) o[0] = (UBYTE)u;
        return 1;
    }
    if (u < 0x800) {
        if (o) { o[0] = (UBYTE)(0xC0 | (u >> 6)); o[1] = (UBYTE)(0x80 | (u & 0x3F)); }
        return 2;
    }
    if (u < 0x10000) {
        if (o) {
            o[0] = (UBYTE)(0xE0 | (u >> 12));
            o[1] = (UBYTE)(0x80 | ((u >> 6) & 0x3F));
            o[2] = (UBYTE)(0x80 | (u & 0x3F));
        }
        return 3;
    }
    if (o) {
        o[0] = (UBYTE)(0xF0 | (u >> 18));
        o[1] = (UBYTE)(0x80 | ((u >> 12) & 0x3F));
        o[2] = (UBYTE)(0x80 | ((u >> 6) & 0x3F));
        o[3] = (UBYTE)(0x80 | (u & 0x3F));
    }
    return 4;
}

/* text from a program as UTF-8 into o (NULL: counted): good UTF-8 as it
 * is, any other byte as Latin-1 (all of them with latin1) */
static LONG tou8(const UBYTE *s, LONG len, int latin1, UBYTE *o)
{
    const UBYTE *e = s + len;
    LONG n = 0;
    int k;
    while (s < e) {
        if (*s < 0x80) {
            if (o) o[n] = *s;
            n++;
            s++;
        } else if (!latin1 && (k = u8seq(s, e)) != 0) {
            if (o) memcpy(o + n, s, k);
            n += k;
            s += k;
        } else
            n += u8put(o ? o + n : NULL, *s++);
    }
    return n;
}

/* Latin Extended-A (U+0100-017F) without its accents */
static const char latinA[] =
    "AaAaAaCcCcCcCcDd" "DdEeEeEeEeEeGgGg" "GgGgHhHhIiIiIiIi" "IiIiJjKkkLlLlLlL"
    "lLlNnNnNnnNnOoOo" "OoOoRrRrRrSsSsSs" "SsTtTtTtUuUuUuUu" "UuUuWwYyYZzZzZzs";

/* a character beyond Latin-1 as the nearest Latin-1 one, else '?'
 * (tui.library's table, without its reversed space) */
static UBYTE fallback(ULONG c)
{
    if (c >= 0x100 && c < 0x180) return (UBYTE)latinA[c - 0x100];
    if (c >= 0x2500 && c < 0x2580) {            /* box lines */
        switch (c) {
        case 0x2500: case 0x2501: case 0x2504: case 0x2505: case 0x2508: case 0x2509:
        case 0x254C: case 0x254D: case 0x2550: case 0x2574: case 0x2576: case 0x2578:
        case 0x257A: case 0x257C: case 0x257E:
            return '-';
        case 0x2502: case 0x2503: case 0x2506: case 0x2507: case 0x250A: case 0x250B:
        case 0x254E: case 0x254F: case 0x2551: case 0x2575: case 0x2577: case 0x2579:
        case 0x257B: case 0x257D: case 0x257F:
            return '|';
        case 0x2571: return '/';
        case 0x2572: return '\\';
        case 0x2573: return 'X';
        }
        return '+';
    }
    if (c >= 0x2580 && c < 0x25A0) return c == 0x2591 ? ':' : '#';
    switch (c) {
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2015:
    case 0x2212: case 0x2043:
        return '-';
    case 0x2018: case 0x2019: case 0x201A: case 0x201B: case 0x2032:
        return '\'';
    case 0x201C: case 0x201D: case 0x201E: case 0x201F: case 0x2033:
        return '"';
    case 0x2039: case 0x2190: case 0x2264: return '<';
    case 0x203A: case 0x2192: case 0x2265: return '>';
    case 0x2191: return '^';
    case 0x2193: return 'v';
    case 0x2022: case 0x2219: case 0x22C5: case 0x2027: return 0xB7;
    case 0x2026: return '.';
    case 0x2713: case 0x2714: return 'v';
    case 0x2717: case 0x2718: return 'x';
    case 0x20AC: return 'E';
    case 0x2122: return 'T';
    case 0x2248: return '~';
    case 0x2260: return '#';
    case 0x2000: case 0x2001: case 0x2002: case 0x2003: case 0x2004: case 0x2005:
    case 0x2006: case 0x2007: case 0x2008: case 0x2009: case 0x200A: case 0x202F:
    case 0x205F: case 0x3000:
        return ' ';
    }
    return '?';
}

/* UTF-8 (as the library keeps it) as Latin-1 into buf, cut to size - 1
 * and ended with a 0; the full length back */
LONG json_tolatin1(const UBYTE *s, LONG len, STRPTR buf, LONG size)
{
    const UBYTE *e = s + len;
    LONG n = 0;
    int k;
    while (s < e) {
        UBYTE c = *s;
        if (c >= 0x80) {
            if ((k = u8seq(s, e)) != 0) {
                ULONG u = u8decode(s, k);
                c = u < 0x100 ? (UBYTE)u : fallback(u);
                s += k;
            } else
                s++;                /* not UTF-8: Latin-1 already */
        } else
            s++;
        if (n < size - 1) buf[n] = (char)c;
        n++;
    }
    if (size > 0) buf[n < size - 1 ? n : size - 1] = 0;
    return n;
}

/* ---- numbers ----------------------------------------------------------- */

static int isdig(UBYTE c) { return c >= '0' && c <= '9'; }

/* the length of a JSON number at s (RFC 8259), or 0 */
static LONG numlen(const UBYTE *s, const UBYTE *e)
{
    const UBYTE *q = s;
    if (q < e && *q == '-') q++;
    if (q >= e) return 0;
    if (*q == '0') q++;
    else if (*q >= '1' && *q <= '9') while (q < e && isdig(*q)) q++;
    else return 0;
    if (q < e && *q == '.') {
        if (++q >= e || !isdig(*q)) return 0;
        while (q < e && isdig(*q)) q++;
    }
    if (q < e && (*q == 'e' || *q == 'E')) {
        q++;
        if (q < e && (*q == '+' || *q == '-')) q++;
        if (q >= e || !isdig(*q)) return 0;
        while (q < e && isdig(*q)) q++;
    }
    return q - s;
}

/* a * 10 + d within lim, without a multiply instruction; 0 if over */
static int step10(ULONG *a, ULONG d, ULONG lim)
{
    ULONG v = *a;
    if (v > 429496729UL) return 0;
    v = (v << 3) + (v << 1);
    if (v + d < v || v + d > lim) return 0;
    *a = v + d;
    return 1;
}

/* a valid JSON number's text times 10^places, rounded half away from
 * zero, kept within a LONG */
static LONG scaled(const UBYTE *t, LONG len, LONG places)
{
    const UBYTE *e = t + len, *ip, *fp = NULL, *q = t;
    LONG nint = 0, nfrac = 0, ex = 0, k, nd, keep, i;
    ULONG lim, acc = 0;
    int neg = 0, eneg = 0;
    if (*q == '-') { neg = 1; q++; }
    lim = neg ? 2147483648UL : 2147483647UL;
    ip = q;
    while (q < e && isdig(*q)) { q++; nint++; }
    if (q < e && *q == '.') {
        fp = ++q;
        while (q < e && isdig(*q)) { q++; nfrac++; }
    }
    if (q < e && (*q == 'e' || *q == 'E')) {
        q++;
        if (*q == '+' || *q == '-') eneg = *q++ == '-';
        while (q < e && isdig(*q)) {
            if (ex < 100000) ex = (ex << 3) + (ex << 1) + (*q - '0');
            q++;
        }
        if (eneg) ex = -ex;
    }
    /* the digits D (integer part, then fraction) and the value
     * D * 10^(ex - nfrac); we want D * 10^k */
    nd = nint + nfrac;
    k = ex - nfrac + places;
    keep = k >= 0 ? nd : nd + k;    /* digits before the point */
    for (i = 0; i < nd && i < keep; i++) {
        UBYTE c = i < nint ? ip[i] : fp[i - nint];
        if (!step10(&acc, c - '0', lim)) { acc = lim; goto done; }
    }
    if (k >= 0) {
        for (i = 0; i < k && acc; i++)
            if (!step10(&acc, 0, lim)) { acc = lim; goto done; }
    } else if (keep >= 0 && keep < nd) {
        UBYTE c = keep < nint ? ip[keep] : fp[keep - nint];
        if (c >= '5' && acc < lim) acc++;
    }
done:
    return neg ? (LONG)(0 - acc) : (LONG)acc;
}

/* a number node's whole value, if it has one */
static void setint(JsonNode *n)
{
    const UBYTE *q = n->str, *e = n->str + n->len;
    ULONG acc = 0, lim;
    int neg = *q == '-';
    n->intok = 0;
    if (neg) q++;
    lim = neg ? 2147483648UL : 2147483647UL;
    for (; q < e; q++) {
        if (!isdig(*q)) return;     /* a fraction or an exponent */
        if (!step10(&acc, *q - '0', lim)) return;
    }
    n->ival = neg ? (LONG)(0 - acc) : (LONG)acc;
    n->intok = 1;
}

/* ---- reading ----------------------------------------------------------- */

typedef struct {
    const UBYTE *s, *p, *e;
    ULONG flags;
    JsonDoc *d;
    const char *msg;
} Rd;

static int fail(Rd *r, const char *msg)
{
    if (!r->msg) r->msg = msg;
    return 0;
}

/* space, and comments where allowed */
static int skipws(Rd *r)
{
    const UBYTE *p = r->p, *e = r->e;
    for (;;) {
        while (p < e && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
        if (p + 1 < e && *p == '/' && (r->flags & JPF_COMMENTS)) {
            if (p[1] == '/') {
                while (p < e && *p != '\n') p++;
                continue;
            }
            if (p[1] == '*') {
                for (p += 2; p + 1 < e && !(p[0] == '*' && p[1] == '/'); p++)
                    ;
                if (p + 1 >= e) {
                    r->p = e;
                    return fail(r, "a comment is not closed");
                }
                p += 2;
                continue;
            }
        }
        r->p = p;
        return 1;
    }
}

static int hex4(const UBYTE *q, ULONG *u)
{
    int i;
    ULONG v = 0;
    for (i = 0; i < 4; i++) {
        UBYTE c = q[i];
        if (c >= '0' && c <= '9') c -= '0';
        else if (c >= 'a' && c <= 'f') c -= 'a' - 10;
        else if (c >= 'A' && c <= 'F') c -= 'A' - 10;
        else return 0;
        v = (v << 4) | c;
    }
    *u = v;
    return 1;
}

/* the string at r->p (on its opening quote) decoded into o (NULL: only
 * counted, and checked); *n its length; r->p after the closing quote */
static int string(Rd *r, UBYTE *o, LONG *len)
{
    const UBYTE *q = r->p + 1, *e = r->e;
    LONG n = 0;
    int k;
    for (;;) {
        UBYTE c;
        if (q >= e) {
            r->p = q;
            return fail(r, "a string is not closed");
        }
        c = *q;
        if (c == '"') break;
        if (c == '\\') {
            ULONG u;
            if (q + 1 >= e) {
                r->p = q;
                return fail(r, "a string is not closed");
            }
            switch (q[1]) {
            case '"': c = '"'; break;
            case '\\': c = '\\'; break;
            case '/': c = '/'; break;
            case 'b': c = 8; break;
            case 'f': c = 12; break;
            case 'n': c = 10; break;
            case 'r': c = 13; break;
            case 't': c = 9; break;
            case 'u':
                if (e - q < 6 || !hex4(q + 2, &u)) {
                    r->p = q;
                    return fail(r, "a \\u escape needs four hex digits");
                }
                q += 6;
                if (u >= 0xD800 && u < 0xDC00) {    /* a pair makes one */
                    ULONG lo;
                    if (e - q >= 6 && q[0] == '\\' && q[1] == 'u' && hex4(q + 2, &lo) &&
                        lo >= 0xDC00 && lo < 0xE000) {
                        u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
                        q += 6;
                    } else
                        u = 0xFFFD;
                } else if (u >= 0xDC00 && u < 0xE000)
                    u = 0xFFFD;                     /* half a pair */
                n += u8put(o ? o + n : NULL, u);
                continue;
            default:
                r->p = q;
                return fail(r, "an unknown \\ escape");
            }
            if (o) o[n] = c;
            n++;
            q += 2;
            continue;
        }
        if (c < 0x20) {
            r->p = q;
            return fail(r, "a control character in a string");
        }
        if (c < 0x80) {
            if (o) o[n] = c;
            n++;
            q++;
        } else if ((k = u8seq(q, e)) != 0) {
            if (o) memcpy(o + n, q, k);
            n += k;
            q += k;
        } else if (r->flags & JPF_LATIN1) {
            n += u8put(o ? o + n : NULL, c);
            q++;
        } else {
            r->p = q;
            return fail(r, "text that is not UTF-8");
        }
    }
    r->p = q + 1;
    *len = n;
    return 1;
}

/* a string into the pool, 0-terminated */
static const UBYTE *poolstring(Rd *r, LONG *len)
{
    const UBYTE *at = r->p;
    UBYTE *o;
    if (!string(r, NULL, len)) return NULL;
    if (!(o = palloc(r->d, *len + 1))) {
        fail(r, "out of memory");
        return NULL;
    }
    r->p = at;
    string(r, o, len);
    o[*len] = 0;
    return o;
}

static int word(Rd *r, const char *w, int n)
{
    if (r->e - r->p < n || memcmp(r->p, w, n)) return 0;
    r->p += n;
    return 1;
}

/* one value at r->p: a scalar, or an empty container to be filled */
static JsonNode *value(Rd *r)
{
    JsonNode *n;
    LONG k;
    if (r->p >= r->e) {
        fail(r, "the text ends too soon");
        return NULL;
    }
    switch (*r->p) {
    case '{':
        n = newnode(r->d, JT_OBJECT);
        r->p++;
        break;
    case '[':
        n = newnode(r->d, JT_ARRAY);
        r->p++;
        break;
    case '"':
        if (!(n = newnode(r->d, JT_STRING))) break;
        if (!(n->str = poolstring(r, &n->len))) return NULL;
        return n;
    case 't':
    case 'f':
    case 'n':
        if (word(r, "true", 4)) {
            if ((n = newnode(r->d, JT_BOOL))) n->ival = 1;
        } else if (word(r, "false", 5))
            n = newnode(r->d, JT_BOOL);
        else if (word(r, "null", 4))
            n = newnode(r->d, JT_NULL);
        else {
            fail(r, "expected a value");
            return NULL;
        }
        break;
    default:
        if (!(k = numlen(r->p, r->e))) {
            fail(r, *r->p == '-' || isdig(*r->p) ? "a number is wrong" : "expected a value");
            return NULL;
        }
        if ((n = newnode(r->d, JT_NUMBER))) {
            UBYTE *t = palloc(r->d, k + 1);
            if (!t) {
                fail(r, "out of memory");
                return NULL;
            }
            memcpy(t, r->p, k);
            t[k] = 0;
            n->str = t;
            n->len = k;
            setint(n);
        }
        r->p += k;
        break;
    }
    if (!n) fail(r, "out of memory");
    return n;
}

static void append(JsonNode *parent, JsonNode *n)
{
    n->parent = parent;
    n->next = NULL;
    if (parent->last) parent->last->next = n;
    else parent->first = n;
    parent->last = n;
    parent->count++;
}

enum { S_VALUE, S_AFTER, S_KEY, S_KEYFIRST, S_VALFIRST };

#define FAILN(m) do { fail(r, m); return NULL; } while (0)

static JsonNode *parse(Rd *r)
{
    JsonNode *cur = NULL, *root = NULL, *n;
    const UBYTE *name = NULL;
    LONG namelen = 0;
    int state = S_VALUE;
    /* a UTF-8 byte order mark at the start says nothing */
    if (r->e - r->p >= 3 && r->p[0] == 0xEF && r->p[1] == 0xBB && r->p[2] == 0xBF) r->p += 3;
    for (;;) {
        if (!skipws(r)) return NULL;
        switch (state) {
        case S_VALFIRST:            /* after [ */
            if (r->p < r->e && *r->p == ']') goto close;
            /* fall through */
        case S_VALUE:
            if (!(n = value(r))) return NULL;
            if (cur) {
                append(cur, n);
                if (cur->type == JT_OBJECT) {
                    n->name = name;
                    n->namelen = namelen;
                }
            } else
                root = n;
            if (n->type == JT_OBJECT || n->type == JT_ARRAY) {
                if (r->p == r->e) {
                    fail(r, "the text ends too soon");
                    return NULL;
                }
                cur = n;
                state = n->type == JT_OBJECT ? S_KEYFIRST : S_VALFIRST;
            } else
                state = S_AFTER;
            break;
        case S_KEYFIRST:            /* after { */
            if (r->p < r->e && *r->p == '}') goto close;
            /* fall through */
        case S_KEY:
            if (r->p >= r->e) FAILN("the text ends too soon");
            if (*r->p != '"') FAILN("expected a name in quotes");
            if (!(name = poolstring(r, &namelen))) return NULL;
            if (!skipws(r)) return NULL;
            if (r->p >= r->e) FAILN("the text ends too soon");
            if (*r->p != ':') FAILN("expected : after a name");
            r->p++;
            state = S_VALUE;
            break;
        case S_AFTER:
            if (!cur) {
                if (r->p != r->e) FAILN("more text after the end");
                return root;
            }
            if (r->p >= r->e) FAILN("the text ends too soon");
            if (*r->p == ',') {
                r->p++;
                if (r->flags & JPF_TRAILING) {
                    if (!skipws(r)) return NULL;
                    if (r->p < r->e && *r->p == (cur->type == JT_OBJECT ? '}' : ']')) goto close;
                }
                state = cur->type == JT_OBJECT ? S_KEY : S_VALUE;
                break;
            }
            if (*r->p == (cur->type == JT_OBJECT ? '}' : ']')) goto close;
            FAILN(cur->type == JT_OBJECT ? "expected , or }" : "expected , or ]");
        }
        continue;
close:
        r->p++;
        cur = cur->parent;
        state = S_AFTER;
    }
}

/* text read into nodes of an existing document (ARexx's JSONSET with
 * JSON in it): the top node, not attached; NULL with err filled in */
JsonNode *json_parse_into(JsonDoc *d, const UBYTE *text, LONG len, ULONG flags, struct JsonError *err)
{
    Rd r;
    JsonNode *n;
    if (err) {
        err->Pos = 0;
        err->Line = err->Col = 1;
        err->Message = NULL;
    }
    r.s = r.p = text;
    r.e = r.s + len;
    r.flags = flags;
    r.d = d;
    r.msg = NULL;
    if ((n = parse(&r)) != NULL) return n;
    if (err) {
        const UBYTE *q, *ls = r.s;
        LONG line = 1;
        for (q = r.s; q < r.p; q++)
            if (*q == '\n') {
                line++;
                ls = q + 1;
            }
        err->Pos = r.p - r.s;
        err->Line = line;
        err->Col = r.p - ls + 1;
        err->Message = r.msg ? r.msg : "out of memory";
    }
    return NULL;
}

JsonDoc *JsonParse(CONST_STRPTR text, LONG len, ULONG flags, struct JsonError *err)
{
    JsonDoc *d;
    if (!text) {
        if (err) {
            err->Pos = 0;
            err->Line = err->Col = 1;
            err->Message = "no text";
        }
        return NULL;
    }
    if (len < 0) len = strlen((const char *)text);
    if (!(d = JsonNewDoc())) {
        if (err) {
            err->Pos = 0;
            err->Line = err->Col = 1;
            err->Message = "out of memory";
        }
        return NULL;
    }
    if ((d->root = json_parse_into(d, (const UBYTE *)text, len, flags, err)) != NULL) return d;
    JsonFree(d);
    return NULL;
}

static JsonDoc *loadfail(struct JsonError *err, const char *msg)
{
    if (err) {
        err->Pos = 0;
        err->Line = err->Col = 1;
        err->Message = msg;
    }
    return NULL;
}

/* a whole file, read in growing pieces (a pipe has no size to ask) */
JsonDoc *JsonLoad(CONST_STRPTR name, ULONG flags, struct JsonError *err)
{
    UBYTE *buf = NULL, *nb;
    LONG size = 0, cap = 0, got;
    JsonDoc *d;
#ifdef __amigaos__
    BPTR fh = Open((STRPTR)name, MODE_OLDFILE);
#else
    BPTR fh = fopen(name, "rb");
#endif
    if (!fh) return loadfail(err, "cannot open the file");
    for (;;) {
        if (size == cap) {
            LONG nc = cap ? cap << 1 : 16384;
            if (nc < cap || !(nb = jalloc(nc))) {
                if (buf) jfree(buf);
#ifdef __amigaos__
                Close(fh);
#else
                fclose(fh);
#endif
                return loadfail(err, "out of memory");
            }
            if (buf) {
                memcpy(nb, buf, size);
                jfree(buf);
            }
            buf = nb;
            cap = nc;
        }
#ifdef __amigaos__
        got = Read(fh, buf + size, cap - size);
#else
        got = (LONG)fread(buf + size, 1, cap - size, fh);
        if (got == 0 && ferror(fh)) got = -1;
#endif
        if (got < 0) {
            jfree(buf);
#ifdef __amigaos__
            Close(fh);
#else
            fclose(fh);
#endif
            return loadfail(err, "cannot read the file");
        }
        if (got == 0) break;
        size += got;
    }
#ifdef __amigaos__
    Close(fh);
#else
    fclose(fh);
#endif
    d = JsonParse((CONST_STRPTR)buf, size, flags, err);
    jfree(buf);
    return d;
}

/* ---- walking ----------------------------------------------------------- */

JsonNode *JsonRoot(JsonDoc *d) { return d ? d->root : NULL; }
LONG JsonType(JsonNode *n) { return n ? n->type : JT_NONE; }
JsonNode *JsonFirst(JsonNode *n) { return n ? n->first : NULL; }
JsonNode *JsonNext(JsonNode *n) { return n ? n->next : NULL; }
JsonNode *JsonParent(JsonNode *n) { return n ? n->parent : NULL; }

LONG JsonCount(JsonNode *n)
{
    return n && (n->type == JT_ARRAY || n->type == JT_OBJECT) ? n->count : 0;
}

CONST_STRPTR JsonName(JsonNode *n)
{
    return n && n->name ? (CONST_STRPTR)n->name : NULL;
}

/* a member by name: the last of that name, as JavaScript does */
static JsonNode *getn(JsonNode *o, const UBYTE *s, LONG len)
{
    JsonNode *m, *hit = NULL;
    if (!o || o->type != JT_OBJECT) return NULL;
    for (m = o->first; m; m = m->next)
        if (m->namelen == len && !memcmp(m->name, s, len)) hit = m;
    return hit;
}

JsonNode *JsonGet(JsonNode *o, CONST_STRPTR name)
{
    if (!name) return NULL;
    return getn(o, (const UBYTE *)name, strlen((const char *)name));
}

JsonNode *JsonAt(JsonNode *a, LONG i)
{
    JsonNode *m;
    if (!a || a->type != JT_ARRAY) return NULL;
    if (i < 0) i += a->count;
    if (i < 0 || i >= a->count) return NULL;
    if (i == a->count - 1) return a->last;
    for (m = a->first; i; i--) m = m->next;
    return m;
}

/* a path: names joined by dots, [n] for an array's n-th (from 0; -1 the
 * last), ["name"] for a name with dots or brackets in it */
JsonNode *JsonFind(JsonNode *n, CONST_STRPTR path)
{
    const UBYTE *s = (const UBYTE *)path, *t;
    if (!s) return NULL;
    while (*s && n) {
        if (*s == '.') {
            s++;
            continue;
        }
        if (*s == '[') {
            s++;
            if (*s == '"') {        /* ["name"], \" and \\ inside */
                JsonNode *m, *hit = NULL;
                const UBYTE *q;
                s++;
                for (q = s; *q && *q != '"'; q++)
                    if (*q == '\\' && q[1]) q++;
                if (*q != '"' || q[1] != ']') return NULL;
                if (n->type != JT_OBJECT) return NULL;
                for (m = n->first; m; m = m->next) {
                    const UBYTE *a = s, *b = m->name, *be = m->name + m->namelen;
                    while (a < q && b < be) {
                        if (*a == '\\') a++;
                        if (*a != *b) break;
                        a++;
                        b++;
                    }
                    if (a == q && b == be) hit = m;
                }
                n = hit;
                s = q + 2;
            } else {                /* [n] */
                LONG i = 0;
                int neg = 0;
                if (*s == '-') {
                    neg = 1;
                    s++;
                }
                if (!isdig(*s)) return NULL;
                while (isdig(*s)) {
                    if (i < 100000000) i = (i << 3) + (i << 1) + (*s - '0');
                    s++;
                }
                if (*s++ != ']') return NULL;
                n = JsonAt(n, neg ? -i : i);
            }
            continue;
        }
        for (t = s; *t && *t != '.' && *t != '['; t++)
            ;
        n = getn(n, s, t - s);
        s = t;
    }
    return n;
}

/* ---- values ------------------------------------------------------------ */

CONST_STRPTR JsonString(JsonNode *n)
{
    return n && n->type == JT_STRING ? (CONST_STRPTR)n->str : NULL;
}

CONST_STRPTR JsonNumText(JsonNode *n)
{
    return n && n->type == JT_NUMBER ? (CONST_STRPTR)n->str : NULL;
}

LONG JsonLength(JsonNode *n)
{
    return n && (n->type == JT_STRING || n->type == JT_NUMBER) ? n->len : 0;
}

/* a number, or a string that is exactly a number ("8.5" from some web
 * services): its text, else NULL */
static const UBYTE *numof(JsonNode *n, LONG *len)
{
    if (!n) return NULL;
    if (n->type == JT_NUMBER) {
        *len = n->len;
        return n->str;
    }
    if (n->type == JT_STRING && n->len && numlen(n->str, n->str + n->len) == n->len) {
        *len = n->len;
        return n->str;
    }
    return NULL;
}

LONG JsonScaled(JsonNode *n, LONG places, LONG def)
{
    const UBYTE *t;
    LONG len;
    if (places < 0 || places > 9 || !(t = numof(n, &len))) return def;
    if (places == 0 && n->type == JT_NUMBER && n->intok) return n->ival;
    return scaled(t, len, places);
}

LONG JsonInt(JsonNode *n, LONG def)
{
    return JsonScaled(n, 0, def);
}

LONG JsonBool(JsonNode *n, LONG def)
{
    return n && n->type == JT_BOOL ? n->ival : def;
}

LONG JsonLatin1(JsonNode *n, STRPTR buf, LONG size)
{
    if (!n || (n->type != JT_STRING && n->type != JT_NUMBER)) {
        if (buf && size > 0) buf[0] = 0;
        return -1;
    }
    return json_tolatin1(n->str, n->len, buf, buf ? size : 0);
}

/* ---- building ---------------------------------------------------------- */

JsonNode *JsonNewObject(JsonDoc *d) { return newnode(d, JT_OBJECT); }
JsonNode *JsonNewArray(JsonDoc *d) { return newnode(d, JT_ARRAY); }
JsonNode *JsonNewNull(JsonDoc *d) { return newnode(d, JT_NULL); }

JsonNode *JsonNewBool(JsonDoc *d, LONG v)
{
    JsonNode *n = newnode(d, JT_BOOL);
    if (n) n->ival = v != 0;
    return n;
}

/* a program's text into the pool as UTF-8 */
static const UBYTE *copyu8(JsonDoc *d, const UBYTE *s, LONG len, int latin1, LONG *outlen)
{
    LONG n = tou8(s, len, latin1, NULL);
    UBYTE *o = palloc(d, n + 1);
    if (!o) return NULL;
    tou8(s, len, latin1, o);
    o[n] = 0;
    *outlen = n;
    return o;
}

JsonNode *JsonNewString(JsonDoc *d, CONST_STRPTR s, LONG len, ULONG flags)
{
    JsonNode *n;
    if (!s) return NULL;
    if (len < 0) len = strlen((const char *)s);
    if (!(n = newnode(d, JT_STRING))) return NULL;
    if (!(n->str = copyu8(d, (const UBYTE *)s, len, flags & JSF_LATIN1, &n->len))) return NULL;
    return n;
}

JsonNode *JsonNewNumber(JsonDoc *d, CONST_STRPTR text)
{
    JsonNode *n;
    UBYTE *t;
    LONG k;
    if (!text) return NULL;
    k = strlen((const char *)text);
    if (!k || numlen((const UBYTE *)text, (const UBYTE *)text + k) != k) return NULL;
    if (!(n = newnode(d, JT_NUMBER)) || !(t = palloc(d, k + 1))) return NULL;
    memcpy(t, text, k + 1);
    n->str = t;
    n->len = k;
    setint(n);
    return n;
}

JsonNode *JsonNewInt(JsonDoc *d, LONG v)
{
    static const ULONG pow10[] = { 1000000000UL, 100000000UL, 10000000UL, 1000000UL,
                                   100000UL, 10000UL, 1000UL, 100UL, 10UL, 1UL };
    char b[12];
    int k = 0, i, started = 0;
    ULONG u = v < 0 ? 0 - (ULONG)v : (ULONG)v;
    JsonNode *n;
    UBYTE *t;
    if (v < 0) b[k++] = '-';
    for (i = 0; i < 10; i++) {      /* digits by subtraction: no divide */
        char c = '0';
        while (u >= pow10[i]) {
            u -= pow10[i];
            c++;
        }
        if (c != '0' || started || i == 9) {
            b[k++] = c;
            started = 1;
        }
    }
    if (!(n = newnode(d, JT_NUMBER)) || !(t = palloc(d, k + 1))) return NULL;
    memcpy(t, b, k);
    t[k] = 0;
    n->str = t;
    n->len = k;
    n->ival = v;
    n->intok = 1;
    return n;
}

static int attached(JsonNode *n)
{
    return n->parent || n->doc->root == n;
}

void JsonRemove(JsonNode *n)
{
    JsonNode *p, *m, *prev = NULL;
    if (!n) return;
    if (n->doc->root == n) n->doc->root = NULL;
    if (!(p = n->parent)) return;
    for (m = p->first; m && m != n; m = m->next) prev = m;
    if (!m) return;
    if (prev) prev->next = n->next;
    else p->first = n->next;
    if (p->last == n) p->last = prev;
    p->count--;
    n->parent = n->next = NULL;
    n->name = NULL;
    n->namelen = 0;
}

/* n (not attached) where old is, with old's name; old taken out */
void json_replace(JsonNode *old, JsonNode *n)
{
    JsonNode *p = old->parent, *m;
    if (old->doc->root == old) {
        old->doc->root = n;
        return;
    }
    if (!p) return;
    n->parent = p;
    n->name = old->name;
    n->namelen = old->namelen;
    n->next = old->next;
    if (p->first == old) p->first = n;
    else {
        for (m = p->first; m->next != old; m = m->next)
            ;
        m->next = n;
    }
    if (p->last == old) p->last = n;
    old->parent = old->next = NULL;
}

/* Latin-1 (ARexx's text) as UTF-8 into a new block of memory, ended with
 * a 0 (jfree it) */
UBYTE *json_latin1_to_u8(const UBYTE *s, LONG len, LONG *outlen)
{
    LONG n = tou8(s, len, 1, NULL);
    UBYTE *o = jalloc(n + 1);
    if (!o) return NULL;
    tou8(s, len, 1, o);
    o[n] = 0;
    if (outlen) *outlen = n;
    return o;
}

/* a JSON number? (ARexx's JSONSET tells numbers from text so) */
LONG json_isnumber(const UBYTE *s, LONG len)
{
    return len > 0 && numlen(s, s + len) == len;
}

void JsonSetRoot(JsonDoc *d, JsonNode *n)
{
    if (!d) return;
    if (n && (n->doc != d || n->parent)) return;
    d->root = n;
}

LONG JsonAdd(JsonNode *parent, CONST_STRPTR name, JsonNode *n)
{
    JsonNode *a;
    if (!parent || !n || n->doc != parent->doc || attached(n)) return FALSE;
    if (parent->type != JT_ARRAY && parent->type != JT_OBJECT) return FALSE;
    for (a = parent; a; a = a->parent)      /* not into itself */
        if (a == n) return FALSE;
    if (parent->type == JT_OBJECT) {
        LONG len;
        const UBYTE *nm;
        if (!name) return FALSE;
        if (!(nm = copyu8(n->doc, (const UBYTE *)name, strlen((const char *)name), 0, &len)))
            return FALSE;
        if ((a = getn(parent, nm, len)) != NULL) JsonRemove(a);   /* a name once */
        append(parent, n);
        n->name = nm;
        n->namelen = len;
    } else
        append(parent, n);
    return TRUE;
}

/* ---- writing ----------------------------------------------------------- */

typedef struct {
    UBYTE *buf;                     /* JsonWrite's buffer, or NULL */
    LONG size, total;
    BPTR fh;                        /* JsonWriteFH's file, or 0 */
    LONG tn;
    int bad;
    ULONG flags;
    UBYTE tmp[512];
} Wr;

static void wflush(Wr *w)
{
    if (w->fh && w->tn) {
#ifdef __amigaos__
        if (Write(w->fh, w->tmp, w->tn) != w->tn) w->bad = 1;
#else
        if (fwrite(w->tmp, 1, w->tn, w->fh) != (size_t)w->tn) w->bad = 1;
#endif
    }
    w->tn = 0;
}

static void wput(Wr *w, const UBYTE *s, LONG n)
{
    if (w->buf) {
        LONG room = w->size - 1 - w->total;
        if (room > 0) memcpy(w->buf + w->total, s, n < room ? n : room);
    } else if (w->fh) {
        while (n > 0) {
            LONG k = (LONG)sizeof(w->tmp) - w->tn;
            if (k > n) k = n;
            memcpy(w->tmp + w->tn, s, k);
            w->tn += k;
            w->total += k;
            s += k;
            n -= k;
            if (w->tn == (LONG)sizeof(w->tmp)) wflush(w);
        }
        return;
    }
    w->total += n;
}

static void wc(Wr *w, UBYTE c) { wput(w, &c, 1); }
static void ws(Wr *w, const char *s) { wput(w, (const UBYTE *)s, strlen(s)); }

static const char hexd[] = "0123456789abcdef";

static void wu(Wr *w, ULONG u)
{
    UBYTE b[6];
    b[0] = '\\';
    b[1] = 'u';
    b[2] = hexd[(u >> 12) & 15];
    b[3] = hexd[(u >> 8) & 15];
    b[4] = hexd[(u >> 4) & 15];
    b[5] = hexd[u & 15];
    wput(w, b, 6);
}

static void wstring(Wr *w, const UBYTE *s, LONG len)
{
    const UBYTE *e = s + len, *run = s;
    int k;
    wc(w, '"');
    while (s < e) {
        UBYTE c = *s;
        if (c >= 0x20 && c != '"' && c != '\\' && (c < 0x80 || !(w->flags & JWF_ASCII))) {
            s++;
            continue;
        }
        wput(w, run, s - run);      /* the plain run before it */
        if (c < 0x80) {
            switch (c) {
            case '"': ws(w, "\\\""); break;
            case '\\': ws(w, "\\\\"); break;
            case 8: ws(w, "\\b"); break;
            case 12: ws(w, "\\f"); break;
            case 10: ws(w, "\\n"); break;
            case 13: ws(w, "\\r"); break;
            case 9: ws(w, "\\t"); break;
            default: wu(w, c);
            }
            s++;
        } else {
            ULONG u;
            if ((k = u8seq(s, e)) != 0) {
                u = u8decode(s, k);
                s += k;
            } else
                u = *s++;           /* kept as Latin-1 */
            if (u >= 0x10000) {
                u -= 0x10000;
                wu(w, 0xD800 + (u >> 10));
                wu(w, 0xDC00 + (u & 0x3FF));
            } else
                wu(w, u);
        }
        run = s;
    }
    wput(w, run, s - run);
    wc(w, '"');
}

static void windent(Wr *w, LONG depth)
{
    static const UBYTE sp[] = "                                ";
    wc(w, '\n');
    depth <<= 1;
    while (depth > 0) {
        LONG k = depth > 32 ? 32 : depth;
        wput(w, sp, k);
        depth -= k;
    }
}

/* the tree under top, without recursion */
static void wtree(Wr *w, JsonNode *top)
{
    JsonNode *n = top;
    LONG depth = 0;
    int pretty = w->flags & JWF_PRETTY;
    for (;;) {
        if (n != top) {
            if (pretty) windent(w, depth);
            if (n->parent->type == JT_OBJECT) {
                wstring(w, n->name, n->namelen);
                ws(w, pretty ? ": " : ":");
            }
        }
        switch (n->type) {
        case JT_OBJECT:
        case JT_ARRAY:
            if (n->first) {
                wc(w, n->type == JT_OBJECT ? '{' : '[');
                depth++;
                n = n->first;
                continue;
            }
            ws(w, n->type == JT_OBJECT ? "{}" : "[]");
            break;
        case JT_STRING: wstring(w, n->str, n->len); break;
        case JT_NUMBER: wput(w, n->str, n->len); break;
        case JT_BOOL: ws(w, n->ival ? "true" : "false"); break;
        default: ws(w, "null");
        }
        while (n != top && !n->next) {
            n = n->parent;
            depth--;
            if (pretty) windent(w, depth);
            wc(w, n->type == JT_OBJECT ? '}' : ']');
        }
        if (n == top) return;
        wc(w, ',');
        n = n->next;
    }
}

LONG JsonWrite(JsonNode *n, STRPTR buf, LONG size, ULONG flags)
{
    Wr w;
    w.buf = (UBYTE *)buf;
    w.size = buf ? size : 0;
    w.total = 0;
    w.fh = 0;
    w.tn = 0;
    w.bad = 0;
    w.flags = flags;
    if (!buf) w.buf = (UBYTE *)"";  /* counting: nothing is written */
    if (n) wtree(&w, n);
    if (buf && size > 0) buf[w.total < size - 1 ? w.total : size - 1] = 0;
    return w.total;
}

LONG JsonWriteFH(JsonNode *n, BPTR fh, ULONG flags)
{
    Wr w;
    if (!n || !fh) return FALSE;
    w.buf = NULL;
    w.size = 0;
    w.total = 0;
    w.fh = fh;
    w.tn = 0;
    w.bad = 0;
    w.flags = flags;
    wtree(&w, n);
    wc(&w, '\n');
    wflush(&w);
    return !w.bad;
}
