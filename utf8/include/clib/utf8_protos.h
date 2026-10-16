/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef CLIB_UTF8_PROTOS_H
#define CLIB_UTF8_PROTOS_H

/*
**   $VER: utf8_protos.h 0.1 $Id: utf8.sfd 0.1 $
**
**   C prototypes. For use with 32 bit integers only.
**
**   Copyright (c) 2001 Amiga, Inc.
**       All Rights Reserved
*/

#include <exec/types.h>
#include <libraries/utf8.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

LONG Utf8Decode(CONST_STRPTR s, LONG len, ULONG * cp);
LONG Utf8Encode(ULONG cp, STRPTR buf);
LONG Utf8Check(CONST_STRPTR s, LONG len);
LONG Utf8Valid(CONST_STRPTR s, LONG len);
LONG Utf8Length(CONST_STRPTR s, LONG len);
LONG Utf8CharWidth(ULONG cp);
LONG Utf8Width(CONST_STRPTR s, LONG len);
LONG Utf8Fit(CONST_STRPTR s, LONG len, LONG cols, LONG * width);
LONG Utf8Next(CONST_STRPTR s, LONG len, LONG pos);
LONG Utf8Prev(CONST_STRPTR s, LONG len, LONG pos);
ULONG Utf8Class(ULONG cp);
ULONG Utf8Upper(ULONG cp);
ULONG Utf8Lower(ULONG cp);
ULONG Utf8NoAccent(ULONG cp);
LONG Utf8Compare(CONST_STRPTR a, LONG alen, CONST_STRPTR b, LONG blen, ULONG flags);
LONG Utf8Find(CONST_STRPTR s, LONG len, CONST_STRPTR pat, LONG plen, ULONG flags, LONG * matchlen);
LONG Utf8Case(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, ULONG flags);
LONG Utf8Normalize(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, ULONG flags);
LONG Utf8ToCharset(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, LONG cs, ULONG flags);
LONG Utf8FromCharset(CONST_STRPTR s, LONG len, STRPTR buf, LONG size, LONG cs);
LONG Utf8Charset(CONST_STRPTR name);
CONST_STRPTR Utf8CharsetName(LONG cs);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_UTF8_PROTOS_H */
