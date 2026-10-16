/* port.h - what fuzzy.library's core needs from the system: memory, files
 * and one setting. On the Amiga exec and dos; on the PC (tests) the C
 * library, so the same match.c and index.c are tested there. */
#ifndef FUZZY_PORT_H
#define FUZZY_PORT_H

#ifdef __amigaos__
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/var.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/fuzzy.h>
#define jalloc(n)  AllocVec((n), MEMF_ANY)
#define jfree(p)   FreeVec(p)
typedef BPTR FH;
#define f_openr(n)        Open((STRPTR)(n), MODE_OLDFILE)
#define f_openw(n)        Open((STRPTR)(n), MODE_NEWFILE)
#define f_read(f, b, n)   Read((f), (b), (n))
#define f_write(f, b, n)  Write((f), (APTR)(b), (n))
#define f_seek0(f, o)     (Seek((f), (o), OFFSET_BEGINNING) >= 0)
#define f_close(f)        Close(f)
#define f_delete(n)       DeleteFile((STRPTR)(n))
#define f_rename(a, b)    Rename((STRPTR)(a), (STRPTR)(b))
#define f_getvar(n, b, s) GetVar((STRPTR)(n), (b), (s), 0)
#ifdef FUZZY_LIB
void *memcpy(void *, const void *, unsigned long);
void *memset(void *, int, unsigned long);
int memcmp(const void *, const void *, unsigned long);
unsigned long strlen(const char *);
#else
#include <string.h>
#endif

#else /* the PC */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint16_t UWORD;
typedef uint8_t UBYTE;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
typedef void *APTR;
typedef FILE *BPTR;
typedef FILE *FH;
#define VOID void
#define TRUE 1
#define FALSE 0
#define jalloc(n)  malloc(n)
#define jfree(p)   free(p)
#define f_openr(n)        fopen((n), "rb")
#define f_openw(n)        fopen((n), "wb")
#define f_read(f, b, n)   ((LONG)fread((b), 1, (n), (f)))
#define f_write(f, b, n)  ((LONG)fwrite((b), 1, (n), (f)))
#define f_seek0(f, o)     (fseek((f), (o), SEEK_SET) == 0)
#define f_close(f)        fclose(f)
#define f_delete(n)       remove(n)
#define f_rename(a, b)    (rename((a), (b)) == 0)
static inline LONG f_getvar(const char *n, char *b, LONG s)
{
    const char *v = getenv("FUZZYFIND_INDEX");
    (void)n;
    if (!v || (LONG)strlen(v) >= s) return -1;
    strcpy(b, v);
    return (LONG)strlen(v);
}
/* the library's own header: its exec/types.h and dos/dos.h are empty
 * stand-ins in tests/pcinc */
#include "../include/libraries/fuzzy.h"
#endif

#define MAXPATH 1024                /* the longest path the index keeps */

/* the core's calls (the library's, and a program's that builds it in) */
FuzzyPattern *FuzzyPrepare(CONST_STRPTR pattern, ULONG flags);
void FuzzyFreePattern(FuzzyPattern *p);
LONG FuzzyScore(FuzzyPattern *p, CONST_STRPTR text, LONG len, UWORD *pos, LONG maxpos);
LONG FuzzyIndexPath(STRPTR buf, LONG size);
struct FuzzyIndex *FuzzyOpenIndex(CONST_STRPTR name, LONG *error);
void FuzzyCloseIndex(struct FuzzyIndex *x);
LONG FuzzyNextEntry(struct FuzzyIndex *x, struct FuzzyEntry *e);
void FuzzyRewind(struct FuzzyIndex *x);
CONST_STRPTR FuzzyVolume(struct FuzzyIndex *x, LONG n, ULONG *date);
struct FuzzyResults *FuzzySearch(struct FuzzyIndex *x, CONST_STRPTR pattern, ULONG flags, LONG max);
void FuzzyFreeResults(struct FuzzyResults *r);
FuzzyWriter *FuzzyNewIndex(CONST_STRPTR name, LONG *error);
LONG FuzzyAddVolume(FuzzyWriter *w, CONST_STRPTR name, ULONG date);
LONG FuzzyAddEntry(FuzzyWriter *w, struct FuzzyEntry *e);
LONG FuzzyEndIndex(FuzzyWriter *w, LONG keep);
CONST_STRPTR FuzzyErrorText(LONG error);

/* inside the core */
LONG fuzzy_nameoffset(const UBYTE *path, LONG len);
LONG fuzzy_score(FuzzyPattern *p, const UBYTE *t, LONG len, LONG name, UWORD *pos, LONG maxpos);
ULONG fuzzy_patmask(FuzzyPattern *p, const ULONG *mtab);

#endif
