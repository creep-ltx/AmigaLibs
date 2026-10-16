/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef CLIB_FUZZY_PROTOS_H
#define CLIB_FUZZY_PROTOS_H

/*
**   $VER: fuzzy_protos.h 0.1 $Id: fuzzy.sfd 0.1 $
**
**   C prototypes. For use with 32 bit integers only.
**
**   Copyright (c) 2001 Amiga, Inc.
**       All Rights Reserved
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/fuzzy.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

FuzzyPattern * FuzzyPrepare(CONST_STRPTR pattern, ULONG flags);
VOID FuzzyFreePattern(FuzzyPattern * pattern);
LONG FuzzyScore(FuzzyPattern * pattern, CONST_STRPTR text, LONG len, UWORD * pos, LONG maxpos);
LONG FuzzyIndexPath(STRPTR buf, LONG size);
struct FuzzyIndex * FuzzyOpenIndex(CONST_STRPTR name, LONG * error);
VOID FuzzyCloseIndex(struct FuzzyIndex * index);
LONG FuzzyNextEntry(struct FuzzyIndex * index, struct FuzzyEntry * entry);
VOID FuzzyRewind(struct FuzzyIndex * index);
CONST_STRPTR FuzzyVolume(struct FuzzyIndex * index, LONG n, ULONG * date);
struct FuzzyResults * FuzzySearch(struct FuzzyIndex * index, CONST_STRPTR pattern, ULONG flags, LONG max);
VOID FuzzyFreeResults(struct FuzzyResults * results);
FuzzyWriter * FuzzyNewIndex(CONST_STRPTR name, LONG * error);
LONG FuzzyAddVolume(FuzzyWriter * writer, CONST_STRPTR name, ULONG date);
LONG FuzzyAddEntry(FuzzyWriter * writer, struct FuzzyEntry * entry);
LONG FuzzyEndIndex(FuzzyWriter * writer, LONG keep);
CONST_STRPTR FuzzyErrorText(LONG error);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_FUZZY_PROTOS_H */
