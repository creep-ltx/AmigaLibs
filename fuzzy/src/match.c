/* match.c - fuzzy.library's matching: fzf's "v1" way, built for a 68000.
 *
 * For each word of the pattern: find its letters in order, left to right
 * (the first window), then from the last one backwards to where the
 * window can start latest (the shortest window ending there), then score
 * that window as fzf does: a point per letter, bonuses for letters at the
 * start of words, after a / or :, in a row, a camelCase capital; small
 * penalties for gaps. One pass and no tables, so it is fast where fzf's
 * "v2" (Smith-Waterman, length x length) would not be. All words must
 * match; their scores add up. A window wholly in the file name (after the
 * last / or :) scores a little more, so a name beats a drawer. Paths are the
 * default (fzf's path scheme: a letter after a space counts as much as after
 * a dot); FPF_TEXT is for other text (history), fzf's default scheme.
 *
 * Letters are Latin-1 (Amiga file names), folded through a table made
 * once per pattern: lower case, and the accent taken off (Göteborg is
 * found by goteborg), unless FPF_CASE / FPF_ACCENTS. No multiply or
 * divide, no floating point. */
#include "port.h"

/* fzf's numbers (algo.go) */
#define SCORE_MATCH        16
#define GAP_START          (-3)
#define GAP_EXTEND         (-1)
#define BONUS_BOUNDARY     8
#define BONUS_NONWORD      8
#define BONUS_CAMEL        7
#define BONUS_CONSECUTIVE  4
#define BONUS_WHITE        10        /* text; paths: BONUS_BOUNDARY, as fzf's path scheme */
#define BONUS_DELIMITER    9
#define BONUS_NAME         16        /* ours: the whole window in the name */

/* character classes, in fzf's order (a class above C_NONWORD is a word) */
enum { C_WHITE, C_NONWORD, C_DELIM, C_LOWER, C_UPPER, C_LETTER, C_NUMBER };

#define MAXWORDS 16

struct Word {
    const UBYTE *s;
    LONG len;
};

struct FuzzyPattern {
    ULONG flags;
    LONG bwhite;                    /* a letter after a space */
    LONG bname;                     /* a window wholly in the file name */
    LONG nwords;
    LONG letters;                   /* all words' letters */
    struct Word w[MAXWORDS];
    UBYTE fold[256];
    UBYTE cls[256];                 /* classof() of every byte, once */
    UBYTE text[1];                  /* the folded words, each 0-ended */
};

static UBYTE classof(UBYTE c)
{
    if (c == ' ' || c == '\t') return C_WHITE;
    if (c == '/' || c == ':' || c == ',' || c == ';' || c == '|') return C_DELIM;
    if (c >= 'a' && c <= 'z') return C_LOWER;
    if (c >= 'A' && c <= 'Z') return C_UPPER;
    if (c >= '0' && c <= '9') return C_NUMBER;
    if (c >= 0xC0 && c != 0xD7 && c != 0xF7) return c < 0xDF ? C_UPPER : C_LOWER;
    if (c == 0xAA || c == 0xB5 || c == 0xBA) return C_LETTER;
    return C_NONWORD;
}

/* Latin-1 0xC0-0xFF without the accent ('.' = keep as it is) */
static const char unaccent[] =
    "AAAAAAACEEEEIIII" "DNOOOOO.OUUUUYTs"
    "aaaaaaaceeeeiiii" "dnooooo.ouuuuyty";

static void mkfold(UBYTE *t, ULONG flags)
{
    int c;
    for (c = 0; c < 256; c++) {
        int f = c;
        if (!(flags & FPF_ACCENTS) && c >= 0xC0 && unaccent[c - 0xC0] != '.')
            f = (UBYTE)unaccent[c - 0xC0];
        if (!(flags & FPF_CASE)) {
            if (f >= 'A' && f <= 'Z') f += 32;
            else if (f >= 0xC0 && f <= 0xDE && f != 0xD7) f += 32;
        }
        t[c] = (UBYTE)f;
    }
}

FuzzyPattern *FuzzyPrepare(CONST_STRPTR pattern, ULONG flags)
{
    const UBYTE *s = (const UBYTE *)pattern;
    LONG n;
    FuzzyPattern *p;
    UBYTE *o;
    if (!s) return NULL;
    n = strlen((const char *)s);
    if (!(p = jalloc(sizeof(*p) + n + MAXWORDS))) return NULL;
    p->flags = flags;
    p->bwhite = flags & FPF_TEXT ? BONUS_WHITE : BONUS_BOUNDARY;
    p->bname = flags & FPF_TEXT ? 0 : BONUS_NAME;
    p->nwords = 0;
    p->letters = 0;
    mkfold(p->fold, flags);
    {
        int c;
        for (c = 0; c < 256; c++) p->cls[c] = classof((UBYTE)c);
    }
    o = p->text;
    while (*s) {
        while (*s == ' ') s++;
        if (!*s) break;
        if (p->nwords == MAXWORDS) break;
        p->w[p->nwords].s = o;
        while (*s && *s != ' ') *o++ = p->fold[*s++];
        p->w[p->nwords].len = o - p->w[p->nwords].s;
        p->letters += p->w[p->nwords].len;
        *o++ = 0;
        p->nwords++;
    }
    return p;
}

void FuzzyFreePattern(FuzzyPattern *p)
{
    if (p) jfree(p);
}

/* where the name starts: after the last / or : */
LONG fuzzy_nameoffset(const UBYTE *path, LONG len)
{
    LONG i = len;
    while (i > 0 && path[i - 1] != '/' && path[i - 1] != ':') i--;
    return i;
}

static LONG bonusfor(int prev, int cls, LONG bwhite)
{
    if (cls > C_NONWORD) {
        if (prev == C_WHITE) return bwhite;
        if (prev == C_DELIM) return BONUS_DELIMITER;
        if (prev == C_NONWORD) return BONUS_BOUNDARY;
    }
    if ((prev == C_LOWER && cls == C_UPPER) || (prev != C_NUMBER && cls == C_NUMBER))
        return BONUS_CAMEL;
    if (cls == C_NONWORD || cls == C_DELIM) return BONUS_NONWORD;
    if (cls == C_WHITE) return bwhite;
    return 0;
}

/* fzf's calculateScore over t[s..e): the word's letters taken greedily */
static LONG wscore(const FuzzyPattern *p, const UBYTE *t, LONG s, LONG e, const struct Word *w,
                   UWORD *pos, LONG *npos, LONG maxpos)
{
    const UBYTE *fold = p->fold, *ctab = p->cls;
    LONG score = 0, bonus, first = 0, consec = 0, i, pi = 0;
    int ingap = 0, prev = s > 0 ? ctab[t[s - 1]] : C_WHITE;
    for (i = s; i < e; i++) {
        UBYTE c = t[i];
        int cls = ctab[c];
        if (pi < w->len && fold[c] == w->s[pi]) {
            score += SCORE_MATCH;
            bonus = bonusfor(prev, cls, p->bwhite);
            if (consec == 0)
                first = bonus;
            else {
                if (bonus >= BONUS_BOUNDARY && bonus > first) first = bonus;
                if (first > bonus) bonus = first;
                if (BONUS_CONSECUTIVE > bonus) bonus = BONUS_CONSECUTIVE;
            }
            score += pi == 0 ? bonus << 1 : bonus;
            if (pos && *npos < maxpos) pos[(*npos)++] = (UWORD)i;
            ingap = 0;
            consec++;
            pi++;
        } else {
            score += ingap ? GAP_EXTEND : GAP_START;
            ingap = 1;
            consec = 0;
            first = 0;
        }
        prev = cls;
    }
    return score;
}

/* fzf v1: the word's letters in order within t[from..to): forwards to the
 * first window, then backwards from its end to the shortest one */
static int window(const UBYTE *fold, const struct Word *w, const UBYTE *t, LONG from, LONG to,
                  LONG *ps, LONG *pe)
{
    const UBYTE *q = t + from, *qe = t + to, *ws = w->s, *we = w->s + w->len, *start = NULL;
    UBYTE want = *ws;
    LONG pi;
    while (q < qe) {                    /* forwards: the letters in order */
        if (fold[*q++] != want) continue;
        if (!start) start = q - 1;
        if (++ws == we) break;
        want = *ws;
    }
    if (ws != we) return 0;
    *pe = q - t;
    pi = w->len - 1;                    /* backwards: the shortest window */
    while (--q >= start)
        if (fold[*q] == w->s[pi] && --pi < 0) break;
    *ps = q - t;
    return 1;
}

/* one word against the text: its best window's score, 0 if it is not there */
static LONG word(const FuzzyPattern *p, const struct Word *w, const UBYTE *t, LONG len, LONG name,
                 UWORD *pos, LONG *npos, LONG maxpos)
{
    const UBYTE *fold = p->fold;
    LONG i, s, e, sc;
    if (w->len == 0) return 1;
    if (p->flags & FPF_PLAIN) {         /* the word as one piece: its best place */
        LONG best = 0, at = -1, k;
        for (i = 0; i + w->len <= len; i++) {
            if (fold[t[i]] != w->s[0]) continue;
            for (k = 1; k < w->len && fold[t[i + k]] == w->s[k]; k++)
                ;
            if (k == w->len) {
                sc = wscore(p, t, i, i + k, w, NULL, NULL, 0);
                if (i >= name) sc += p->bname;
                if (sc > best || at < 0) {
                    best = sc;
                    at = i;
                }
            }
        }
        if (at < 0) return 0;
        if (pos) wscore(p, t, at, at + w->len, w, pos, npos, maxpos);
        return best > 0 ? best : 1;
    }
    if (!window(fold, w, t, 0, len, &s, &e)) return 0;
    sc = wscore(p, t, s, e, w, NULL, NULL, 0);
    if (s >= name)
        sc += p->bname;
    else {                              /* the name alone may do better */
        LONG ns, ne, nsc;
        if (name > 0 && window(fold, w, t, name, len, &ns, &ne)) {
            nsc = wscore(p, t, ns, ne, w, NULL, NULL, 0) + p->bname;
            if (nsc > sc) {
                sc = nsc;
                s = ns;
                e = ne;
            }
        }
    }
    if (pos) wscore(p, t, s, e, w, pos, npos, maxpos);
    return sc > 0 ? sc : 1;             /* thin, but a match */
}

/* FuzzyScore() with the name's start known (FuzzySearch has it) */
LONG fuzzy_score(FuzzyPattern *p, const UBYTE *t, LONG len, LONG name, UWORD *pos, LONG maxpos)
{
    LONG total = 0, i, npos = 0, sc;
    if (p->nwords == 0) return 1;       /* nothing typed: everything matches */
    for (i = 0; i < p->nwords; i++) {
        sc = word(p, &p->w[i], t, len, name, pos, &npos, maxpos);
        if (sc <= 0) return 0;
        total += sc;
    }
    return total;
}

LONG FuzzyScore(FuzzyPattern *p, CONST_STRPTR text, LONG len, UWORD *pos, LONG maxpos)
{
    const UBYTE *t = (const UBYTE *)text;
    if (!p || !t) return 0;
    if (len < 0) len = strlen(text);
    if (len > 65535) len = 65535;       /* positions are UWORDs */
    return fuzzy_score(p, t, len, fuzzy_nameoffset(t, len), pos, maxpos);
}

/* the pattern's letter fingerprint (index.c's table): every letter of every
 * word; a text without one of them cannot match */
ULONG fuzzy_patmask(FuzzyPattern *p, const ULONG *mtab)
{
    ULONG m = 0;
    LONG i, k;
    for (i = 0; i < p->nwords; i++)
        for (k = 0; k < p->w[i].len; k++) m |= mtab[p->w[i].s[k]];
    return m;
}
