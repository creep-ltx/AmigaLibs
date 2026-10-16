/* libraries/utf8.h - utf8.library: UTF-8 text on a Latin-1 Amiga.
 *
 * Reading and writing characters (Utf8Decode, Utf8Encode), what a text
 * is (Utf8Check, Utf8Valid, Utf8Length), how many columns it takes on a
 * terminal (Utf8CharWidth, Utf8Width, Utf8Fit), moving a cursor by whole
 * characters (Utf8Next, Utf8Prev), what a character is (Utf8Class,
 * Utf8Upper, Utf8Lower, Utf8NoAccent), comparing and searching with or
 * without case and accents (Utf8Compare, Utf8Find, Utf8Case), accents
 * put together or apart (Utf8Normalize), and converting to and from
 * Latin-1 and thirteen other 8-bit sets (Utf8ToCharset, Utf8FromCharset,
 * Utf8Charset, Utf8CharsetName).
 *
 * Every call takes text as a pointer and a length in bytes; a length
 * below 0 means "up to the 0 byte". A byte that is not UTF-8 is taken
 * as the Latin-1 character it would be on the Amiga, so Amiga text
 * passes through unharmed.
 *
 * ARexx: the library is also an ARexx function library
 * (ADDLIB('utf8.library', 0, -30, 0)); see the guide.
 *
 * The rule of a library: a function's place in the table never moves.
 * New calls are added at the end of utf8.sfd and the revision goes up. */
#ifndef LIBRARIES_UTF8_H
#define LIBRARIES_UTF8_H

#include <exec/types.h>

#define UTF8NAME     "utf8.library"
#define UTF8VERSION  0          /* the version and revision this header */
#define UTF8REVISION 1          /* describes: 0.1 (while the version is 0,
                                 * check lib_Revision for newer calls) */

/* Utf8Check() */
#define U8_ASCII     0          /* only bytes below 0x80 */
#define U8_UTF8      1          /* good UTF-8 with characters beyond ASCII */
#define U8_LATIN1    2          /* has bytes that are not UTF-8: 8-bit text */

/* Utf8Class() bits */
#define UC_LETTER    0x0001
#define UC_DIGIT     0x0002     /* 0-9 in any script */
#define UC_NUMBER    0x0004     /* other numbers: ½ ² Ⅻ */
#define UC_PUNCT     0x0008     /* punctuation and symbols, emoji too */
#define UC_SPACE     0x0010     /* spaces, tab, line ends */
#define UC_MARK      0x0020     /* accents and other marks that join */
#define UC_CONTROL   0x0040     /* controls, formats, unassigned, private */
#define UC_UPPER     0x0100     /* has a lower case form */
#define UC_LOWER     0x0200     /* has an upper case form */
#define UC_WIDE      0x0400     /* two columns */
#define UC_WORD      (UC_LETTER | UC_DIGIT | UC_NUMBER | UC_MARK)

/* Utf8Compare(), Utf8Find() flags */
#define U8F_NOCASE   0x0001     /* A = a, Ä = ä, Σ = σ */
#define U8F_NOACCENT 0x0002     /* é = e, Å = A, ł = l */

/* Utf8Case() flags */
#define U8F_UPPER    0x0010
#define U8F_LOWER    0x0020

/* Utf8Normalize() flags */
#define U8F_COMPOSE   0x0100    /* a + ring above -> å (NFC: what Amigas
                                 * and Windows use) */
#define U8F_DECOMPOSE 0x0200    /* å -> a + ring above (NFD: what Macs
                                 * may give in file names) */

/* Utf8ToCharset() flags */
#define U8F_STRICT   0x1000     /* '?' for what the set lacks; without it
                                 * the nearest: é -> e, " for “, ... for … */

/* character sets (Utf8ToCharset, Utf8FromCharset, Utf8Charset) */
#define U8CS_LATIN1   0         /* ISO-8859-1: the Amiga's own */
#define U8CS_LATIN2   1         /* ISO-8859-2: Central Europe (Polish,
                                 * Czech, Hungarian Amigas) */
#define U8CS_LATIN9   2         /* ISO-8859-15: Latin-1 with € */
#define U8CS_CYRILLIC 3         /* ISO-8859-5 */
#define U8CS_GREEK    4         /* ISO-8859-7 */
#define U8CS_LATIN5   5         /* ISO-8859-9: Turkish */
#define U8CS_CP1250   6         /* Windows Central Europe */
#define U8CS_CP1251   7         /* Windows Cyrillic */
#define U8CS_CP1252   8         /* Windows Western */
#define U8CS_CP437    9         /* the PC's own: BBS and ANSI art */
#define U8CS_CP850    10        /* PC Western */
#define U8CS_CP866    11        /* PC Cyrillic */
#define U8CS_KOI8R    12        /* Russian */
#define U8CS_MACROMAN 13        /* old Macintosh */
#define U8CS_ASCII    14        /* 7-bit: everything else as the nearest */
#define U8CS_UTF8     15        /* UTF-8 itself: a checked copy, so one code
                                 * path serves any set a file names */
#define U8CS_COUNT    16

#endif
