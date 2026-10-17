/* tables.h - the Unicode tables (tables.c, written by tools/gen.py). */
#ifndef U8_TABLES_H
#define U8_TABLES_H

#include "port.h"

extern const char u8_unicode_version[];
extern const ULONG u8_zero_lo[], u8_zero_hi[], u8_wide_lo[], u8_wide_hi[];
extern const LONG u8_zero_n, u8_wide_n;
extern const UWORD u8_class_at[];
extern const UBYTE u8_class_kind[];
extern const LONG u8_class_n;
extern const ULONG u8_upper_at[], u8_lower_at[];
extern const UWORD u8_upper_len[], u8_lower_len[];
extern const LONG u8_upper_delta[], u8_lower_delta[];
extern const LONG u8_upper_n, u8_lower_n;
extern const UWORD u8_marks[], u8_comp_ch[], u8_comp_base[];
extern const UBYTE u8_comp_mark[];
extern const UWORD u8_comp_byb[];
extern const LONG u8_marks_n, u8_comp_n;
extern const UWORD u8_sets[];
extern const LONG u8_sets_n;

#endif
