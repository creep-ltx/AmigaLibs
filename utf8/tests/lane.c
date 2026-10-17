/* lane.c - Utf8ToCharset's quick lane for other scripts against the full
 * way (the same source built with -DNOQUICKLANE): every code point alone,
 * after a letter and before one, into Latin-1 and ASCII, written out for
 * cmp.   make lanecheck */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/port.h"

int main(void)
{
    static const char *ctx[] = { "%s", "a%sb", "%s\xcc\x81", "\xe3\x81\x8d%s", "%s\xe3\x82\x83" };
    char one[8], in[32], out[512];
    /* (each line: code point, context, set, length, result, results cut at 2, 4, 6) */
    ULONG c;
    int k, cs;
    for (c = 0x80; c < 0x110000; c++) {
        if (c >= 0xD800 && c < 0xE000) continue;
        one[Utf8Encode(c, one)] = 0;
        for (k = 0; k < 5; k++)
            for (cs = 0; cs < 2; cs++) {
                snprintf(in, sizeof(in), ctx[k], one);
                LONG n = Utf8ToCharset(in, -1, out, sizeof(out), cs ? U8CS_ASCII : U8CS_LATIN1, 0);
                char cut[8];
                int m;
                printf("%lx %d %d %ld %s", (unsigned long)c, k, cs, (long)n, out);
                for (m = 2; m <= 6; m += 2) {   /* and cut short */
                    Utf8ToCharset(in, -1, cut, m, cs ? U8CS_ASCII : U8CS_LATIN1, 0);
                    printf(" |%s", cut);
                }
                printf("\n");
            }
    }
    /* and random strings of the pieces that matter: accents to join, kana
     * that join, broken bytes, letters each road takes */
    {
        static const char *pc[] = {
            "a", "Z", " ", "\n", "\xc3\xa4", "\xc2\xa0", "\xc2\x85", "\xd0\x96", "\xd1\x89",
            "\xd1\xa0", "\xce\xa9", "\xce\xac", "\xcf\x82", "\xe6\x97\xa5", "\xe4\xb7\x80",
            "\xe3\x81\x8d", "\xe3\x82\x83", "\xe3\x81\xa3", "\xe3\x83\xbc", "\xea\xb0\x80",
            "\xf0\x9f\x98\x80", "\xcc\x81", "\xcd\x84", "\xe2\x80\x9c", "\xe2\x94\x80",
            "\xe1\x85\xa1", "\xe3\x82\x99", "\xff", "\xe6\x97"
        };
        char mix[256], cut[16];
        int it, i, m;
        srand(7);
        for (it = 0; it < 400000; it++) {
            int pieces = 1 + rand() % 12;
            mix[0] = 0;
            for (i = 0; i < pieces; i++) strcat(mix, pc[rand() % (int)(sizeof(pc) / sizeof(pc[0]))]);
            for (cs = 0; cs < 2; cs++) {
                LONG n = Utf8ToCharset(mix, -1, out, sizeof(out), cs ? U8CS_ASCII : U8CS_LATIN1, 0);
                printf("%ld %s", (long)n, out);
                for (m = 1; m < 12; m += 2) {
                    Utf8ToCharset(mix, -1, cut, m, cs ? U8CS_ASCII : U8CS_LATIN1, 0);
                    printf("|%s", cut);
                }
                printf("\n");
            }
        }
    }
    return 0;
}
