/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef _INLINE_UTF8_H
#define _INLINE_UTF8_H

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

#ifndef UTF8_BASE_NAME
#define UTF8_BASE_NAME Utf8Base
#endif /* !UTF8_BASE_NAME */

#define Utf8Decode(___s, ___len, ___cp) \
      LP3(0x24, LONG, Utf8Decode , CONST_STRPTR, ___s, a0, LONG, ___len, d0, ULONG *, ___cp, a1,\
      , UTF8_BASE_NAME)

#define Utf8Encode(___cp, ___buf) \
      LP2(0x2a, LONG, Utf8Encode , ULONG, ___cp, d0, STRPTR, ___buf, a0,\
      , UTF8_BASE_NAME)

#define Utf8Check(___s, ___len) \
      LP2(0x30, LONG, Utf8Check , CONST_STRPTR, ___s, a0, LONG, ___len, d0,\
      , UTF8_BASE_NAME)

#define Utf8Valid(___s, ___len) \
      LP2(0x36, LONG, Utf8Valid , CONST_STRPTR, ___s, a0, LONG, ___len, d0,\
      , UTF8_BASE_NAME)

#define Utf8Length(___s, ___len) \
      LP2(0x3c, LONG, Utf8Length , CONST_STRPTR, ___s, a0, LONG, ___len, d0,\
      , UTF8_BASE_NAME)

#define Utf8CharWidth(___cp) \
      LP1(0x42, LONG, Utf8CharWidth , ULONG, ___cp, d0,\
      , UTF8_BASE_NAME)

#define Utf8Width(___s, ___len) \
      LP2(0x48, LONG, Utf8Width , CONST_STRPTR, ___s, a0, LONG, ___len, d0,\
      , UTF8_BASE_NAME)

#define Utf8Fit(___s, ___len, ___cols, ___width) \
      LP4(0x4e, LONG, Utf8Fit , CONST_STRPTR, ___s, a0, LONG, ___len, d0, LONG, ___cols, d1, LONG *, ___width, a1,\
      , UTF8_BASE_NAME)

#define Utf8Next(___s, ___len, ___pos) \
      LP3(0x54, LONG, Utf8Next , CONST_STRPTR, ___s, a0, LONG, ___len, d0, LONG, ___pos, d1,\
      , UTF8_BASE_NAME)

#define Utf8Prev(___s, ___len, ___pos) \
      LP3(0x5a, LONG, Utf8Prev , CONST_STRPTR, ___s, a0, LONG, ___len, d0, LONG, ___pos, d1,\
      , UTF8_BASE_NAME)

#define Utf8Class(___cp) \
      LP1(0x60, ULONG, Utf8Class , ULONG, ___cp, d0,\
      , UTF8_BASE_NAME)

#define Utf8Upper(___cp) \
      LP1(0x66, ULONG, Utf8Upper , ULONG, ___cp, d0,\
      , UTF8_BASE_NAME)

#define Utf8Lower(___cp) \
      LP1(0x6c, ULONG, Utf8Lower , ULONG, ___cp, d0,\
      , UTF8_BASE_NAME)

#define Utf8NoAccent(___cp) \
      LP1(0x72, ULONG, Utf8NoAccent , ULONG, ___cp, d0,\
      , UTF8_BASE_NAME)

#define Utf8Compare(___a, ___alen, ___b, ___blen, ___flags) \
      LP5(0x78, LONG, Utf8Compare , CONST_STRPTR, ___a, a0, LONG, ___alen, d0, CONST_STRPTR, ___b, a1, LONG, ___blen, d1, ULONG, ___flags, d2,\
      , UTF8_BASE_NAME)

#define Utf8Find(___s, ___len, ___pat, ___plen, ___flags, ___matchlen) \
      LP6(0x7e, LONG, Utf8Find , CONST_STRPTR, ___s, a0, LONG, ___len, d0, CONST_STRPTR, ___pat, a1, LONG, ___plen, d1, ULONG, ___flags, d2, LONG *, ___matchlen, a2,\
      , UTF8_BASE_NAME)

#define Utf8Case(___s, ___len, ___buf, ___size, ___flags) \
      LP5(0x84, LONG, Utf8Case , CONST_STRPTR, ___s, a0, LONG, ___len, d0, STRPTR, ___buf, a1, LONG, ___size, d1, ULONG, ___flags, d2,\
      , UTF8_BASE_NAME)

#define Utf8Normalize(___s, ___len, ___buf, ___size, ___flags) \
      LP5(0x8a, LONG, Utf8Normalize , CONST_STRPTR, ___s, a0, LONG, ___len, d0, STRPTR, ___buf, a1, LONG, ___size, d1, ULONG, ___flags, d2,\
      , UTF8_BASE_NAME)

#define Utf8ToCharset(___s, ___len, ___buf, ___size, ___cs, ___flags) \
      LP6(0x90, LONG, Utf8ToCharset , CONST_STRPTR, ___s, a0, LONG, ___len, d0, STRPTR, ___buf, a1, LONG, ___size, d1, LONG, ___cs, d2, ULONG, ___flags, d3,\
      , UTF8_BASE_NAME)

#define Utf8FromCharset(___s, ___len, ___buf, ___size, ___cs) \
      LP5(0x96, LONG, Utf8FromCharset , CONST_STRPTR, ___s, a0, LONG, ___len, d0, STRPTR, ___buf, a1, LONG, ___size, d1, LONG, ___cs, d2,\
      , UTF8_BASE_NAME)

#define Utf8Charset(___name) \
      LP1(0x9c, LONG, Utf8Charset , CONST_STRPTR, ___name, a0,\
      , UTF8_BASE_NAME)

#define Utf8CharsetName(___cs) \
      LP1(0xa2, CONST_STRPTR, Utf8CharsetName , LONG, ___cs, d0,\
      , UTF8_BASE_NAME)

#endif /* !_INLINE_UTF8_H */
