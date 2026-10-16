/* dump.c - every code point through utf8.c, for tools/check.py to hold
 * against Python's Unicode database (make pccheck). One line each:
 * code point, width, upper, lower, base, class (hex). Then the 8-bit
 * sets: "S set byte codepoint". */
#include <stdio.h>
#include "../src/port.h"

int main(void)
{
    ULONG c;
    LONG cs;
    int b;
    for (c = 0; c < 0x110000; c++)
        printf("%lx %ld %lx %lx %lx %lx\n", (unsigned long)c, (long)Utf8CharWidth(c),
               (unsigned long)Utf8Upper(c), (unsigned long)Utf8Lower(c),
               (unsigned long)Utf8NoAccent(c), (unsigned long)Utf8Class(c));
    for (cs = 0; cs < U8CS_COUNT; cs++)
        for (b = 0x80; b < 0x100; b++) {
            char one[2] = { (char)b, 0 }, u8[8];
            ULONG d;
            Utf8FromCharset(one, 1, u8, sizeof(u8), cs);
            Utf8Decode(u8, -1, &d);
            printf("S %ld %x %lx\n", (long)cs, b, (unsigned long)d);
        }
    /* accents apart and together: "N code point, NFD, NFC of the NFD" */
    for (c = 0xC0; c < 0xD7A4; c++) {
        char one[8], d[32], k[32];
        int i;
        if (c == 0x3100) c = 0xAC00;
        one[Utf8Encode(c, one)] = 0;
        Utf8Normalize(one, -1, d, sizeof(d), U8F_DECOMPOSE);
        Utf8Normalize(d, -1, k, sizeof(k), U8F_COMPOSE);
        printf("N %lx ", (unsigned long)c);
        for (i = 0; d[i]; i++) printf("%02x", (UBYTE)d[i]);
        printf(" ");
        for (i = 0; k[i]; i++) printf("%02x", (UBYTE)k[i]);
        printf("\n");
    }
    return 0;
}
