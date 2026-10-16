/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef _INLINE_FUZZY_H
#define _INLINE_FUZZY_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef FUZZY_BASE_NAME
#define FUZZY_BASE_NAME FuzzyBase
#endif /* !FUZZY_BASE_NAME */

#define FuzzyPrepare(___pattern, ___flags) \
      LP2(0x24, FuzzyPattern *, FuzzyPrepare , CONST_STRPTR, ___pattern, a0, ULONG, ___flags, d0,\
      , FUZZY_BASE_NAME)

#define FuzzyFreePattern(___pattern) \
      LP1NR(0x2a, FuzzyFreePattern , FuzzyPattern *, ___pattern, a0,\
      , FUZZY_BASE_NAME)

#define FuzzyScore(___pattern, ___text, ___len, ___pos, ___maxpos) \
      LP5(0x30, LONG, FuzzyScore , FuzzyPattern *, ___pattern, a0, CONST_STRPTR, ___text, a1, LONG, ___len, d0, UWORD *, ___pos, a2, LONG, ___maxpos, d1,\
      , FUZZY_BASE_NAME)

#define FuzzyIndexPath(___buf, ___size) \
      LP2(0x36, LONG, FuzzyIndexPath , STRPTR, ___buf, a0, LONG, ___size, d0,\
      , FUZZY_BASE_NAME)

#define FuzzyOpenIndex(___name, ___error) \
      LP2(0x3c, struct FuzzyIndex *, FuzzyOpenIndex , CONST_STRPTR, ___name, a0, LONG *, ___error, a1,\
      , FUZZY_BASE_NAME)

#define FuzzyCloseIndex(___index) \
      LP1NR(0x42, FuzzyCloseIndex , struct FuzzyIndex *, ___index, a0,\
      , FUZZY_BASE_NAME)

#define FuzzyNextEntry(___index, ___entry) \
      LP2(0x48, LONG, FuzzyNextEntry , struct FuzzyIndex *, ___index, a0, struct FuzzyEntry *, ___entry, a1,\
      , FUZZY_BASE_NAME)

#define FuzzyRewind(___index) \
      LP1NR(0x4e, FuzzyRewind , struct FuzzyIndex *, ___index, a0,\
      , FUZZY_BASE_NAME)

#define FuzzyVolume(___index, ___n, ___date) \
      LP3(0x54, CONST_STRPTR, FuzzyVolume , struct FuzzyIndex *, ___index, a0, LONG, ___n, d0, ULONG *, ___date, a1,\
      , FUZZY_BASE_NAME)

#define FuzzySearch(___index, ___pattern, ___flags, ___max) \
      LP4(0x5a, struct FuzzyResults *, FuzzySearch , struct FuzzyIndex *, ___index, a0, CONST_STRPTR, ___pattern, a1, ULONG, ___flags, d0, LONG, ___max, d1,\
      , FUZZY_BASE_NAME)

#define FuzzyFreeResults(___results) \
      LP1NR(0x60, FuzzyFreeResults , struct FuzzyResults *, ___results, a0,\
      , FUZZY_BASE_NAME)

#define FuzzyNewIndex(___name, ___error) \
      LP2(0x66, FuzzyWriter *, FuzzyNewIndex , CONST_STRPTR, ___name, a0, LONG *, ___error, a1,\
      , FUZZY_BASE_NAME)

#define FuzzyAddVolume(___writer, ___name, ___date) \
      LP3(0x6c, LONG, FuzzyAddVolume , FuzzyWriter *, ___writer, a0, CONST_STRPTR, ___name, a1, ULONG, ___date, d0,\
      , FUZZY_BASE_NAME)

#define FuzzyAddEntry(___writer, ___entry) \
      LP2(0x72, LONG, FuzzyAddEntry , FuzzyWriter *, ___writer, a0, struct FuzzyEntry *, ___entry, a1,\
      , FUZZY_BASE_NAME)

#define FuzzyEndIndex(___writer, ___keep) \
      LP2(0x78, LONG, FuzzyEndIndex , FuzzyWriter *, ___writer, a0, LONG, ___keep, d0,\
      , FUZZY_BASE_NAME)

#define FuzzyErrorText(___error) \
      LP1(0x7e, CONST_STRPTR, FuzzyErrorText , LONG, ___error, d0,\
      , FUZZY_BASE_NAME)

#endif /* !_INLINE_FUZZY_H */
