/* libraries/fuzzy.h - fuzzy.library: fuzzy matching, and the FuzzyFind
 * index (a catalogue of every file and drawer on the chosen disks).
 *
 * Matching: FuzzyPrepare() a pattern once, then FuzzyScore() it against
 * any number of texts: 0 is no match, higher is better. "csrc" matches
 * "config/source.c"; letters at the start of words, letters in a row and
 * letters in the file name count more. Case and accents do not matter
 * unless asked. Several words (a space between) must all match.
 *
 * The index: FuzzyOpenIndex() and FuzzyNextEntry() read every entry;
 * FuzzySearch() returns the best matches; FuzzyNewIndex(), FuzzyAddEntry()
 * and FuzzyEndIndex() write one (FuzzyFind's updatedb does). Format 1 is
 * described in the guide.
 *
 * ARexx: also an ARexx function library (ADDLIB('fuzzy.library', 0, -30, 0)).
 *
 * The rule of a library: a function's place in the table never moves.
 * New calls are added at the end of fuzzy.sfd and the revision goes up. */
#ifndef LIBRARIES_FUZZY_H
#define LIBRARIES_FUZZY_H

#include <exec/types.h>
#include <dos/dos.h>

#define FUZZYNAME     "fuzzy.library"
#define FUZZYVERSION  0         /* the version and revision this header */
#define FUZZYREVISION 1         /* describes: 0.1 */

/* where the index is unless the ENV: setting FuzzyFind/Index says else */
#define FUZZY_INDEX   "S:FuzzyFind.index"

/* ---- matching ---------------------------------------------------------- */

typedef struct FuzzyPattern FuzzyPattern;   /* private */

/* FuzzyPrepare() flags */
#define FPF_CASE     0x01       /* case matters */
#define FPF_ACCENTS  0x02       /* accents matter (otherwise o finds ö) */
#define FPF_PLAIN    0x04       /* each word as one piece of text, not
                                 * letters with gaps between */
#define FPF_TEXT     0x08       /* the texts are not paths (history,
                                 * commands): no file-name bonus, and a
                                 * word after a space counts more */

/* ---- the index --------------------------------------------------------- */

/* one file or drawer */
struct FuzzyEntry {
    CONST_STRPTR Path;          /* "Work:Music/mod.foo", Latin-1, 0-ended */
    LONG PathLen;
    LONG NameOffset;            /* where the name starts in Path */
    UBYTE Type;                 /* FET_ */
    UBYTE Prot;                 /* protection bits hsparwed */
    UWORD Pad;
    ULONG Size;                 /* bytes (0 for a drawer) */
    ULONG Date;                 /* seconds since 1978-01-01, as AmigaDOS */
};

/* FuzzyEntry.Type */
#define FET_FILE     0
#define FET_DRAWER   1
#define FET_LINK     2          /* a soft link (not followed) */

/* an open index: read these, never write them */
struct FuzzyIndex {
    LONG Count;                 /* entries */
    ULONG Created;              /* when updatedb made it (seconds since 1978) */
    LONG Volumes;               /* how many volumes it covers */
};

/* the results of FuzzySearch(), best first: read, never write */
struct FuzzyHit {
    CONST_STRPTR Path;
    LONG PathLen;
    LONG NameOffset;
    LONG Score;
    UBYTE Type;
    UBYTE Prot;
    UWORD Pad;
    ULONG Size;
    ULONG Date;
};

struct FuzzyResults {
    LONG Count;                 /* hits in Hits */
    LONG Matched;               /* how many entries matched in all */
    struct FuzzyHit *Hits;
};

/* FuzzySearch() flags (and FPF_ above) */
#define FSF_FILES    0x0100     /* only files */
#define FSF_DRAWERS  0x0200     /* only drawers */
#define FSF_NAME     0x0400     /* match only the name, not the drawers above */

/* errors (FuzzyOpenIndex, FuzzyNewIndex, FuzzyEndIndex) */
#define FERR_OK       0
#define FERR_NOFILE   1         /* the file is not there (or cannot be made) */
#define FERR_NOTINDEX 2         /* not a FuzzyFind index */
#define FERR_VERSION  3         /* an index format this library does not know */
#define FERR_NOMEM    4
#define FERR_IO       5         /* reading or writing failed */
#define FERR_ORDER    6         /* FuzzyAddVolume() after the first entry */

typedef struct FuzzyWriter FuzzyWriter;     /* private */

#endif
