/* unit.c - utf8.c on the PC, under the sanitizers: every call, the edges.
 *   make pctest */
#include <stdio.h>
#include "../src/port.h"

static int fails, checks;

#define CHECK(c) do { checks++; if (!(c)) { printf("line %d: %s\n", __LINE__, #c); fails++; } } while (0)
#define STR(a, b) (strcmp((const char *)(a), (const char *)(b)) == 0)

int main(void)
{
    char buf[256], small[6];
    ULONG c;
    LONG n, w, ml;
    int i;

    /* Decode, Encode */
    CHECK(Utf8Decode("A", -1, &c) == 1 && c == 'A');
    CHECK(Utf8Decode("\xc3\xa5", 2, &c) == 2 && c == 0xE5);
    CHECK(Utf8Decode("\xe2\x82\xac", -1, &c) == 3 && c == 0x20AC);
    CHECK(Utf8Decode("\xf0\x9f\x98\x80", 4, &c) == 4 && c == 0x1F600);
    CHECK(Utf8Decode("\xe5", 1, &c) == -1 && c == 0xE5);            /* Latin-1 å */
    CHECK(Utf8Decode("\xc3", 1, &c) == -1 && c == 0xC3);            /* cut short */
    CHECK(Utf8Decode("\xe2\x82", -1, &c) == -1 && c == 0xE2);       /* ended by the 0 */
    CHECK(Utf8Decode("\xc0\x80", 2, &c) == -1);                     /* too long a way */
    CHECK(Utf8Decode("\xed\xa0\x80", 3, &c) == -1);                 /* a surrogate */
    CHECK(Utf8Decode("\xf4\x90\x80\x80", 4, &c) == -1);             /* past U+10FFFF */
    CHECK(Utf8Decode("", -1, &c) == 0);
    CHECK(Utf8Decode(NULL, 5, &c) == 0);
    CHECK(Utf8Encode(0xE5, buf) == 2 && (UBYTE)buf[0] == 0xC3 && (UBYTE)buf[1] == 0xA5);
    CHECK(Utf8Encode(0x1F600, buf) == 4);
    CHECK(Utf8Encode(0xD800, buf) == 0);
    CHECK(Utf8Encode(0x110000, buf) == 0);
    for (c = 0; c < 0x110000; c += 7) {
        ULONG d;
        if (c >= 0xD800 && c < 0xE000) continue;
        n = Utf8Encode(c, buf);
        if (Utf8Decode(buf, n, &d) != n || d != c) { CHECK(!"round trip"); break; }
    }

    /* Check, Valid, Length */
    CHECK(Utf8Check("plain", -1) == U8_ASCII);
    CHECK(Utf8Check("G\xc3\xb6teborg", -1) == U8_UTF8);
    CHECK(Utf8Check("G\xf6teborg", -1) == U8_LATIN1);
    CHECK(Utf8Valid("ab\xc3\xa5", -1) == -1);
    CHECK(Utf8Valid("ab\xe5x", -1) == 2);
    CHECK(Utf8Valid("ab\xe2\x82", -1) == 2);
    CHECK(Utf8Length("G\xc3\xb6teborg", -1) == 8);
    CHECK(Utf8Length("G\xf6teborg", -1) == 8);
    CHECK(Utf8Length("a\xcc\x8a", -1) == 1);                        /* a + ring: one */
    CHECK(Utf8Length("\xf0\x9f\x87\xb8\xf0\x9f\x87\xaa", -1) == 1); /* the Swedish flag */
    CHECK(Utf8Length("\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7", -1) == 1);
    CHECK(Utf8Length("\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbd", -1) == 1); /* thumbs up, a skin tone */
    CHECK(Utf8Length("\r\n\r\n", -1) == 2);
    CHECK(Utf8Length("\xea\xb0\x80", -1) == 1);                     /* Hangul ga */
    CHECK(Utf8Length("\xe1\x84\x80\xe1\x85\xa1", -1) == 1);         /* the same, as jamo */

    /* widths */
    CHECK(Utf8CharWidth('A') == 1);
    CHECK(Utf8CharWidth('\t') == -1);
    CHECK(Utf8CharWidth(0x9B) == -1);
    CHECK(Utf8CharWidth(0xE5) == 1);
    CHECK(Utf8CharWidth(0x301) == 0);
    CHECK(Utf8CharWidth(0x200D) == 0);
    CHECK(Utf8CharWidth(0x65E5) == 2);                              /* 日 */
    CHECK(Utf8CharWidth(0x1F600) == 2);
    CHECK(Utf8CharWidth(0xFF21) == 2);                              /* fullwidth A */
    CHECK(Utf8CharWidth(0xD800) == -1);
    CHECK(Utf8Width("abc", -1) == 3);
    CHECK(Utf8Width("\xe6\x97\xa5\xe6\x9c\xac", -1) == 4);          /* 日本 */
    CHECK(Utf8Width("e\xcc\x81", -1) == 1);                         /* e + acute */
    CHECK(Utf8Width("\xf0\x9f\x87\xb8\xf0\x9f\x87\xaa", -1) == 2);
    CHECK(Utf8Width("\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9", -1) == 2);
    CHECK(Utf8Width("a\tb", -1) == 2);                              /* controls: 0 */

    /* Fit */
    CHECK(Utf8Fit("abcdef", -1, 3, &w) == 3 && w == 3);
    CHECK(Utf8Fit("\xe6\x97\xa5\xe6\x9c\xac", -1, 3, &w) == 3 && w == 2);  /* half of 本 no */
    CHECK(Utf8Fit("e\xcc\x81x", -1, 1, &w) == 3 && w == 1);         /* the accent with it */
    CHECK(Utf8Fit("abc", -1, 0, &w) == 0 && w == 0);
    CHECK(Utf8Fit("abc", -1, 10, &w) == 3 && w == 3);

    /* Next, Prev */
    {
        const char *t = "a\xc3\xa5" "e\xcc\x81\xcc\xa3" "\xf0\x9f\x87\xb8\xf0\x9f\x87\xaa" "\r\nz";
        LONG len = strlen(t), pos[16], k = 0, p = 0;
        while (p < len) {
            pos[k++] = p;
            p = Utf8Next(t, len, p);
        }
        CHECK(k == 6);
        CHECK(pos[0] == 0 && pos[1] == 1 && pos[2] == 3 && pos[3] == 8 && pos[4] == 16 && pos[5] == 18);
        CHECK(Utf8Prev(t, len, len) == 18);
        for (i = k - 1; i > 0; i--) CHECK(Utf8Prev(t, len, pos[i]) == pos[i - 1]);
        CHECK(Utf8Prev(t, len, 0) == 0);
        CHECK(Utf8Prev(t, len, 5) == 3);                            /* inside: its start */
        CHECK(Utf8Next(t, len, len) == len);
    }
    /* flags pair from the start of a run */
    {
        const char *t = "\xf0\x9f\x87\xb8\xf0\x9f\x87\xaa\xf0\x9f\x87\xab\xf0\x9f\x87\xae";
        CHECK(Utf8Prev(t, 16, 16) == 8);
        CHECK(Utf8Next(t, 16, 0) == 8);
    }

    /* Class, Upper, Lower, Base */
    CHECK(Utf8Class('a') == (UC_LETTER | UC_LOWER));
    CHECK(Utf8Class('A') == (UC_LETTER | UC_UPPER));
    CHECK(Utf8Class('7') == UC_DIGIT);
    CHECK(Utf8Class(' ') == UC_SPACE);
    CHECK(Utf8Class('\n') == (UC_SPACE | UC_CONTROL));
    CHECK(Utf8Class('!') == UC_PUNCT);
    CHECK(Utf8Class(0xC5) == (UC_LETTER | UC_UPPER));
    CHECK(Utf8Class(0x301) == UC_MARK);
    CHECK(Utf8Class(0x65E5) == (UC_LETTER | UC_WIDE));
    CHECK(Utf8Class(0x1F600) == (UC_PUNCT | UC_WIDE));
    CHECK(Utf8Class(0x0663) == UC_DIGIT);                           /* Arabic 3 */
    CHECK(Utf8Class(0xBD) == UC_NUMBER);
    CHECK(Utf8Class(0x3000) == (UC_SPACE | UC_WIDE));
    CHECK(Utf8Upper(0xE5) == 0xC5 && Utf8Lower(0xC5) == 0xE5);
    CHECK(Utf8Upper(0x3C3) == 0x3A3 && Utf8Upper(0x3C2) == 0x3A3); /* σ ς -> Σ */
    CHECK(Utf8Lower(0x416) == 0x436);                               /* Ж -> ж */
    CHECK(Utf8Upper(0x101) == 0x100 && Utf8Upper(0x100) == 0x100);  /* ā Ā: a step-2 run */
    CHECK(Utf8Upper(0xDF) == 0xDF);                                 /* ß has no one-letter upper */
    CHECK(Utf8Upper(0xFF) == 0x178);
    CHECK(Utf8NoAccent(0xE5) == 'a' && Utf8NoAccent(0xC9) == 'E' && Utf8NoAccent(0x142) == 'l');
    CHECK(Utf8NoAccent(0x1EC7) == 'e');                                 /* ệ: two accents */
    CHECK(Utf8NoAccent(0x419) == 0x418);                                /* Й -> И */
    CHECK(Utf8NoAccent('x') == 'x' && Utf8NoAccent(0x65E5) == 0x65E5);

    /* Compare */
    CHECK(Utf8Compare("abc", -1, "abc", -1, 0) == 0);
    CHECK(Utf8Compare("abc", -1, "abd", -1, 0) < 0);
    CHECK(Utf8Compare("ab", -1, "abc", -1, 0) < 0);
    CHECK(Utf8Compare("\xc3\xa5", -1, "a\xcc\x8a", -1, 0) == 0);    /* å either way */
    CHECK(Utf8Compare("\xc3\xa5", -1, "\xe5", -1, 0) == 0);         /* and as Latin-1 */
    CHECK(Utf8Compare("Malm\xc3\xb6", -1, "MALM\xc3\x96", -1, U8F_NOCASE) == 0);
    CHECK(Utf8Compare("Malm\xc3\xb6", -1, "malmo", -1, U8F_NOCASE) != 0);
    CHECK(Utf8Compare("Malm\xc3\xb6", -1, "malmo", -1, U8F_NOCASE | U8F_NOACCENT) == 0);
    CHECK(Utf8Compare("\xce\xa3", -1, "\xcf\x82", -1, U8F_NOCASE) == 0);
    CHECK(Utf8Compare(NULL, 0, "", -1, 0) == 0);
    CHECK(Utf8Compare("h\xcd\x84", -1, "h\xcc\x88\xcc\x81", -1, 0) == 0);  /* U+0344 is two marks */

    /* Find */
    CHECK(Utf8Find("hello world", -1, "world", -1, 0, &ml) == 6 && ml == 5);
    CHECK(Utf8Find("hello", -1, "x", -1, 0, &ml) == -1 && ml == 0);
    CHECK(Utf8Find("G\xc3\xb6teborg", -1, "GOTE", -1, U8F_NOCASE | U8F_NOACCENT, &ml) == 0 && ml == 5);
    CHECK(Utf8Find("Ka\xcc\x8aseberga", -1, "\xc3\xa5s", -1, 0, &ml) == 1 && ml == 4);
    CHECK(Utf8Find("e\xcc\x81t\xc3\xa9", -1, "e", -1, 0, &ml) == -1);  /* é is not e */
    CHECK(Utf8Find("e\xcc\x81t\xc3\xa9", -1, "e", -1, U8F_NOACCENT, &ml) == 0 && ml == 3);
    CHECK(Utf8Find("abc", -1, "", -1, 0, &ml) == 0 && ml == 0);
    CHECK(Utf8Find("", -1, "a", -1, 0, &ml) == -1);
    CHECK(Utf8Find("xx\xe5yy", -1, "\xc3\xa5", -1, 0, &ml) == 2 && ml == 1);

    /* Case */
    CHECK(Utf8Case("G\xc3\xb6teborg", -1, buf, sizeof(buf), U8F_UPPER) == 9 && STR(buf, "G\xc3\x96TEBORG"));
    CHECK(Utf8Case("\xd0\x96\xd0\x98\xd0\x92", -1, buf, sizeof(buf), U8F_LOWER) == 6 &&
          STR(buf, "\xd0\xb6\xd0\xb8\xd0\xb2"));
    CHECK(Utf8Case("\xe5", -1, buf, sizeof(buf), U8F_UPPER) == 2 && STR(buf, "\xc3\x85"));  /* Latin-1 in, UTF-8 out */
    /* too small: whole characters only, the length all of it needs */
    CHECK(Utf8Case("ab\xc3\xa5\xc3\xa5", -1, small, 4, 0) == 6 && STR(small, "ab"));
    CHECK(Utf8Case("abc", -1, NULL, 0, U8F_UPPER) == 3);

    /* Normalize */
    CHECK(Utf8Normalize("a\xcc\x8a" "e\xcc\x81", -1, buf, sizeof(buf), U8F_COMPOSE) == 4 &&
          STR(buf, "\xc3\xa5\xc3\xa9"));
    CHECK(Utf8Normalize("\xc3\xa5", -1, buf, sizeof(buf), U8F_DECOMPOSE) == 3 && STR(buf, "a\xcc\x8a"));
    CHECK(Utf8Normalize("\xe1\xbb\x87", -1, buf, sizeof(buf), U8F_DECOMPOSE) == 5 &&
          STR(buf, "e\xcc\xa3\xcc\x82"));
    CHECK(Utf8Normalize("e\xcc\xa3\xcc\x82", -1, buf, sizeof(buf), U8F_COMPOSE) == 3 && STR(buf, "\xe1\xbb\x87"));
    CHECK(Utf8Normalize("\xea\xb0\x81", -1, buf, sizeof(buf), U8F_DECOMPOSE) == 9);         /* 각 */
    CHECK(Utf8Normalize(buf, -1, small, sizeof(small), U8F_COMPOSE) == 3 && STR(small, "\xea\xb0\x81"));
    CHECK(Utf8Normalize("\xe3\x81\x8b\xe3\x82\x99", -1, buf, sizeof(buf), U8F_COMPOSE) == 3 &&
          STR(buf, "\xe3\x81\x8c"));                                /* か + ゛ -> が */
    CHECK(Utf8Normalize("x\xe5", -1, buf, sizeof(buf), 0) == 3 && STR(buf, "x\xc3\xa5"));

    /* character sets */
    CHECK(Utf8ToCharset("G\xc3\xb6teborg", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 8 && STR(buf, "G\xf6teborg"));
    CHECK(Utf8ToCharset("Ka\xcc\x8a", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 2 && STR(buf, "K\xe5"));
    CHECK(Utf8ToCharset("\xe2\x80\x9chi\xe2\x80\x9d\xe2\x80\xa6", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 7 &&
          STR(buf, "\"hi\"..."));
    CHECK(Utf8ToCharset("\xe2\x82\xac" "5", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 4 && STR(buf, "EUR5"));
    CHECK(Utf8ToCharset("\xe2\x82\xac", -1, buf, sizeof(buf), U8CS_LATIN9, 0) == 1 && (UBYTE)buf[0] == 0xA4);
    CHECK(Utf8ToCharset("\xc5\x81\xc3\xb3" "d\xc5\xba", -1, buf, sizeof(buf), U8CS_LATIN2, 0) == 4 &&
          STR(buf, "\xa3\xf3" "d\xbc"));                            /* Łódź */
    CHECK(Utf8ToCharset("\xc5\x81\xc3\xb3" "d\xc5\xba", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 4 &&
          STR(buf, "L\xf3" "dz"));
    CHECK(Utf8ToCharset("\xc5\x81" "x", -1, buf, sizeof(buf), U8CS_LATIN1, U8F_STRICT) == 2 && STR(buf, "?x"));
    /* other scripts spelt in Latin letters (0.1b2) */
    {   /* every Hangul syllable: its three parts as plain division gives them */
        static const char *L[19] = { "g", "kk", "n", "d", "tt", "r", "m", "b", "pp", "s", "ss", "", "j", "jj", "ch", "k", "t", "p", "h" };
        static const char *V[21] = { "a", "ae", "ya", "yae", "eo", "e", "yeo", "ye", "o", "wa", "wae", "oe", "yo", "u", "wo", "we", "wi", "yu", "eu", "ui", "i" };
        static const char *T[28] = { "", "k", "k", "k", "n", "n", "n", "t", "l", "k", "m", "l", "l", "l", "p", "l", "m", "p", "p", "t", "t", "ng", "t", "t", "k", "t", "p", "t" };
        ULONG sy;
        int ok = 1;
        for (sy = 0; sy < 11172; sy++) {
            char one[8], want[16];
            one[Utf8Encode(0xAC00 + sy, one)] = 0;
            strcpy(want, L[sy / 588]);
            strcat(want, V[sy % 588 / 28]);
            strcat(want, T[sy % 28]);
            Utf8ToCharset(one, -1, buf, sizeof(buf), U8CS_LATIN1, 0);
            if (!STR(buf, want)) ok = 0;
        }
        CHECK(ok);
    }
    CHECK(Utf8ToCharset("\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82, \xd0\xbc\xd0\xb8\xd1\x80", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "Privet, mir"));
    CHECK(Utf8ToCharset("\xd0\x96\xd0\xa3\xd0\x9a \xd0\xa9\xd1\x83\xd0\xba\xd0\xb0 \xd0\x81\xd0\xbb\xd0\xba\xd0\xb0 \xd0\xbe\xd0\xb1\xd1\x8a\xd1\x8f\xd0\xb2\xd0\xbb\xd0\xb5\xd0\xbd\xd0\xb8\xd0\xb5", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "ZhUK Shchuka Yolka ob\"yavlenie"));
    CHECK(Utf8ToCharset("\xd0\x9a\xd0\xb8\xd1\x97\xd0\xb2 \xd2\x90\xd0\xb0\xd0\xbd\xd0\xbe\xd0\xba", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "Kiyiv Ganok"));
    CHECK(Utf8ToCharset("\xce\xa9\xce\xbc\xce\xad\xce\xb3\xce\xb1 \xce\x91\xce\xb8\xce\xae\xce\xbd\xce\xb1 \xe1\xbc\x84\xce\xbd\xce\xb8\xcf\x81\xcf\x89\xcf\x80\xce\xbf\xcf\x82", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "Omega Athina anthropos"));
    CHECK(Utf8ToCharset("\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x81\xaf", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "konnichiha"));
    CHECK(Utf8ToCharset("\xe3\x82\xab\xe3\x82\xbf\xe3\x82\xab\xe3\x83\x8a", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "katakana"));
    CHECK(Utf8ToCharset("\xe3\x81\x8c\xe3\x81\xa3\xe3\x81\x93\xe3\x81\x86", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "gakkou"));
    CHECK(Utf8ToCharset("\xe3\x81\x8d\xe3\x81\xa3\xe3\x81\xa6 \xe3\x81\xbe\xe3\x81\xa3\xe3\x81\xa1\xe3\x82\x83", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "kitte matcha"));
    CHECK(Utf8ToCharset("\xe3\x81\x97\xe3\x82\x83\xe3\x81\x97\xe3\x82\x93 \xe3\x81\xa1\xe3\x82\x87\xe3\x81\xa3\xe3\x81\xa8 \xe3\x81\x98\xe3\x82\x85\xe3\x81\x86 \xe3\x81\xb2\xe3\x82\x83\xe3\x81\x8f", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "shashin chotto juu hyaku"));
    CHECK(Utf8ToCharset("\xe3\x82\xb3\xe3\x83\xbc\xe3\x83\x92\xe3\x83\xbc", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "koohii"));
    CHECK(Utf8ToCharset("\xe3\x83\x95\xe3\x82\xa1\xe3\x82\xa4\xe3\x83\xab \xe3\x83\x86\xe3\x82\xa3\xe3\x83\xbc \xe3\x82\xa6\xe3\x82\xa3\xe3\x82\xad \xe3\x82\xb7\xe3\x82\xa7\xe3\x82\xa2", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "fairu tii wiki shea"));
    CHECK(Utf8ToCharset("\xe3\x81\x93\xe3\x82\x93\xe3\x82\x84 \xe3\x81\x8d\xe3\x82\x93\xe3\x81\x88\xe3\x82\x93", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "kon'ya kin'en"));
    CHECK(Utf8ToCharset("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "???"));
    CHECK(Utf8ToCharset("\xec\x95\x88\xeb\x85\x95\xed\x95\x98\xec\x84\xb8\xec\x9a\x94 \xec\x84\x9c\xec\x9a\xb8", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "annyeonghaseyo seoul"));
    CHECK(Utf8ToCharset("\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbd \xf0\x9f\x87\xb8\xf0\x9f\x87\xaa", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, "? SE"));
    CHECK(Utf8ToCharset("\xe3\x82\x9d", -1, buf, sizeof(buf), U8CS_LATIN1, 0) >= 0 && STR(buf, ""));
    CHECK(Utf8ToCharset("\xd0\x96", -1, buf, sizeof(buf), U8CS_CYRILLIC, 0) == 1 && (UBYTE)buf[0] == 0xB6);   /* a set with the letter keeps it */
    CHECK(Utf8ToCharset("\xd0\x96", -1, buf, sizeof(buf), U8CS_LATIN1, U8F_STRICT) == 1 && STR(buf, "?"));
    CHECK(Utf8ToCharset("\xe6\x97\xa5", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 1 && STR(buf, "?"));
    CHECK(Utf8ToCharset("\xe2\x94\x8c\xe2\x94\x80\xe2\x94\x90", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 3 &&
          STR(buf, "+-+"));
    CHECK(Utf8ToCharset("\xe2\x94\x8c\xe2\x94\x80", -1, buf, sizeof(buf), U8CS_CP437, 0) == 2 &&
          STR(buf, "\xda\xc4"));
    CHECK(Utf8ToCharset("\xc3\xa9t\xc3\xa9", -1, buf, sizeof(buf), U8CS_ASCII, 0) == 3 && STR(buf, "ete"));
    CHECK(Utf8ToCharset("\xf0\x9f\x91\x8d\xef\xb8\x8f", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 1);  /* the selector dropped */
    CHECK(Utf8ToCharset("\xe5", -1, buf, sizeof(buf), U8CS_LATIN1, 0) == 1 && (UBYTE)buf[0] == 0xE5);
    CHECK(Utf8ToCharset("x", -1, buf, sizeof(buf), 99, 0) == -1);
    /* U+0344 is two marks: ä and the second, which must not be lost at the end */
    CHECK(Utf8ToCharset("a\xcd\x84", -1, buf, sizeof(buf), U8CS_UTF8, 0) == 4 && STR(buf, "\xc3\xa4\xcc\x81"));
    CHECK(Utf8ToCharset("\xd0\x96", -1, buf, sizeof(buf), U8CS_KOI8R, 0) == 1 && (UBYTE)buf[0] == 0xF6);
    CHECK(Utf8FromCharset("G\xf6teborg", -1, buf, sizeof(buf), U8CS_LATIN1) == 9 && STR(buf, "G\xc3\xb6teborg"));
    CHECK(Utf8FromCharset("\xa4", -1, buf, sizeof(buf), U8CS_LATIN9) == 3 && STR(buf, "\xe2\x82\xac"));
    CHECK(Utf8FromCharset("\xda\xc4", -1, buf, sizeof(buf), U8CS_CP437) == 6 &&
          STR(buf, "\xe2\x94\x8c\xe2\x94\x80"));
    CHECK(Utf8FromCharset("\x80", -1, buf, sizeof(buf), U8CS_ASCII) == 3 && STR(buf, "\xef\xbf\xbd"));
    CHECK(Utf8FromCharset("a\xe5\xc3\xa5", -1, buf, sizeof(buf), U8CS_UTF8) == 5 && STR(buf, "a\xc3\xa5\xc3\xa5"));
    /* every set's every byte there and back */
    for (n = 0; n < U8CS_COUNT; n++) {
        if (n == U8CS_UTF8) continue;
        for (i = 0x20; i < 0x100; i++) {
            char one[2] = { (char)i, 0 }, u8[8], back[8];
            LONG k = Utf8FromCharset(one, 1, u8, sizeof(u8), n);
            if (!strcmp(u8, "\xef\xbf\xbd")) continue;          /* the set has nothing there */
            if (Utf8ToCharset(u8, k, back, sizeof(back), n, U8F_STRICT) != 1 || (UBYTE)back[0] != i) {
                printf("set %ld byte %02x: no round trip\n", (long)n, i);
                fails++;
            }
        }
    }

    /* every stand-in is found (the table is searched by halving) */
    {
        static const ULONG some[] = { 0xA9, 0x152, 0x2026, 0x20AC, 0x2122, 0x2264, 0x2588, 0x3000, 0xFB02 };
        for (i = 0; i < (int)(sizeof(some) / sizeof(some[0])); i++) {
            char one[8];
            one[Utf8Encode(some[i], one)] = 0;
            Utf8ToCharset(one, -1, buf, sizeof(buf), U8CS_ASCII, 0);
            CHECK(buf[0] != '?');
        }
    }

    /* names */
    CHECK(Utf8Charset("ISO-8859-1") == U8CS_LATIN1);
    CHECK(Utf8Charset("iso_8859-2") == U8CS_LATIN2);
    CHECK(Utf8Charset("Latin9") == U8CS_LATIN9);
    CHECK(Utf8Charset("CP437") == U8CS_CP437);
    CHECK(Utf8Charset("utf8") == U8CS_UTF8);
    CHECK(Utf8Charset("Windows-1252") == U8CS_CP1252);
    CHECK(Utf8Charset("klingon") == -1);
    CHECK(Utf8Charset(NULL) == -1);
    CHECK(STR(Utf8CharsetName(U8CS_KOI8R), "KOI8-R"));
    CHECK(Utf8CharsetName(U8CS_COUNT) == NULL);
    for (n = 0; n < U8CS_COUNT; n++) CHECK(Utf8Charset(Utf8CharsetName(n)) == n);

    printf("%d checks, %d failed\n", checks, fails);
    return fails != 0;
}
