/* utf8test - utf8.library tried on the Amiga, through the library: the
 * same checks as the PC's tests/unit.c, then how fast the common calls
 * are. Prints what failed; returns 0 when all passed, 10 otherwise (20
 * without the library).
 *
 *   utf8test */
#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utf8.h>

struct Library *Utf8Base;

/* unit.c's checks, each call now going through the library */
#define U8_PORT_H
#define main unit_main
#include "unit.c"
#undef main

/* ticks (1/50 s) since t */
static LONG since(struct DateStamp *t)
{
    struct DateStamp n;
    DateStamp(&n);
    return (n.ds_Days - t->ds_Days) * 24 * 60 * 50 * 60 + (n.ds_Minute - t->ds_Minute) * 60 * 50 +
           (n.ds_Tick - t->ds_Tick);
}

static void speed(const char *what, const char *text, LONG len, int call)
{
    static char out[8192];
    struct DateStamp t;
    LONG n = 0, ticks, w;
    DateStamp(&t);
    do {
        switch (call) {
        case 0: Utf8Width(text, len); break;
        case 1: Utf8Length(text, len); break;
        case 2: Utf8ToCharset(text, len, out, sizeof(out), U8CS_LATIN1, 0); break;
        case 3: for (w = 0; w < len; w = Utf8Next(text, len, w)) ; break;
        case 4: Utf8Find(text, len, "zebra", -1, U8F_NOCASE, &w); break;
        case 5: { volatile LONG sum = 0; const UBYTE *q = (const UBYTE *)text, *e = q + len;
                  while (q < e) sum += *q++; } break;
        }
        n++;
    } while ((ticks = since(&t)) < 100);
    /* (in steps that cannot overflow on a fast machine: Emu68 does millions) */
    printf("  %-36s %5ld KB/s\n", what, (long)((n * (len >> 6)) / ticks * 50 / 16));
}

static char sv[4096], cjk[4096], plain[4096];

static void fill(char *t, const char *piece)
{
    int i = 0, k = strlen(piece);
    while (i + k < 4095) {
        memcpy(t + i, piece, k);
        i += k;
    }
    t[i] = 0;
}

static void speeds(const char *name, int call)
{
    char what[40];
    sprintf(what, "%s, Swedish", name);
    speed(what, sv, strlen(sv), call);
    sprintf(what, "%s, ASCII", name);
    speed(what, plain, strlen(plain), call);
    sprintf(what, "%s, CJK+emoji", name);
    speed(what, cjk, strlen(cjk), call);
}

int main(void)
{
    int r;
    if (!(Utf8Base = OpenLibrary(UTF8NAME, 0))) {
        printf("utf8test: needs utf8.library\n");
        return 20;
    }
    printf("utf8.library %d.%d\n", Utf8Base->lib_Version, Utf8Base->lib_Revision);
    r = unit_main();

    /* 4 KB each: Swedish (UTF-8), English, Japanese with emoji */
    fill(sv, "G\xc3\xb6teborg \xc3\xa4r en stad p\xc3\xa5 v\xc3\xa4stkusten. ");
    fill(plain, "Gothenburg is a city on the west coast. ");
    fill(cjk, "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x81\xae\xe6\x96\x87 \xf0\x9f\x98\x80 ");
    printf("speed (each call on 4 KB, for 2 seconds each):\n");
    speed("(a plain byte loop, to compare)", plain, strlen(plain), 5);
    speeds("Utf8Width", 0);
    speeds("Utf8Length", 1);
    speeds("Utf8ToCharset Latin-1", 2);
    speeds("Utf8Next one by one", 3);
    speeds("Utf8Find NOCASE", 4);
    CloseLibrary(Utf8Base);
    return r ? 10 : 0;
}
