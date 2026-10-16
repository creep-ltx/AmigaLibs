/* port.h - what utf8.c needs from the system: next to nothing. On the
 * Amiga exec's types; on the PC (tests, fuzzing) the C library's, so the
 * same utf8.c is tested there. */
#ifndef U8_PORT_H
#define U8_PORT_H

#ifdef __amigaos__
#include <exec/types.h>
#include <libraries/utf8.h>
#ifdef UTF8_LIB
/* the library has no C library: lib.c has these */
void *memcpy(void *, const void *, unsigned long);
unsigned long strlen(const char *);
#else
#include <string.h>
#endif

#else /* the PC */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int16_t WORD;
typedef uint16_t UWORD;
typedef uint8_t UBYTE;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
typedef void *APTR;
#define VOID void
#define TRUE 1
#define FALSE 0
/* the library's own header: its exec/types.h is an empty stand-in in
 * tests/pcinc */
#include "../include/libraries/utf8.h"
#endif

/* utf8.c's calls (the library's, and a program's that builds it in) */
LONG Utf8Decode(CONST_STRPTR s, LONG len, ULONG *cp);
LONG Utf8Encode(ULONG cp, STRPTR buf);
LONG Utf8Check(CONST_STRPTR s, LONG len);
LONG Utf8Valid(CONST_STRPTR s, LONG len);
LONG Utf8Length(CONST_STRPTR s, LONG len);
LONG Utf8CharWidth(ULONG cp);
LONG Utf8Width(CONST_STRPTR s, LONG len);
LONG Utf8Fit(CONST_STRPTR s, LONG len, LONG cols, LONG *width);
LONG Utf8Next(CONST_STRPTR s, LONG len, LONG pos);
LONG Utf8Prev(CONST_STRPTR s, LONG len, LONG pos);
ULONG Utf8Class(ULONG cp);
ULONG Utf8Upper(ULONG cp);
ULONG Utf8Lower(ULONG cp);
ULONG Utf8NoAccent(ULONG cp);
LONG Utf8Compare(CONST_STRPTR a, LONG alen, CONST_STRPTR b, LONG blen, ULONG flags);
LONG Utf8Find(CONST_STRPTR s, LONG len, CONST_STRPTR pat, LONG plen, ULONG flags, LONG *matchlen);
LONG Utf8Case(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, ULONG flags);
LONG Utf8Normalize(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, ULONG flags);
LONG Utf8ToCharset(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, LONG cs, ULONG flags);
LONG Utf8FromCharset(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, LONG cs);
LONG Utf8Charset(CONST_STRPTR name);
CONST_STRPTR Utf8CharsetName(LONG cs);

#endif
