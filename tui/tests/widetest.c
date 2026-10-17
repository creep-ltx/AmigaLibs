/* widetest - tui.library 0.5's wide characters and accents, to look at
 * (in CTerm, and in a plain console): each line says what it should
 * show. Draws, waits SECS seconds (default 30) or a key, closes.
 *
 *   widetest [SECS] */
#include <stdio.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "tui.h"

static void label(int r, const char *s)
{
    tui_put(0, r, s, strlen(s), 8, TC_DEFAULT, 0);
}

int main(int argc, char **argv)
{
    static const char jp[] = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";     /* 日本語 */
    static const ULONG jpc[] = { 0x65E5, 0x672C, 0x8A9E };
    TuiEv e;
    ULONG got;
    LONG n, secs = argc > 1 ? atol(argv[1]) : 30;
    int i, x;
    char buf[80];

    if (!tui_open()) {
        printf("widetest: needs a terminal\n");
        return 20;
    }
    tui_clear();
    sprintf(buf, "tui %s %s, widths: e%ld \xe6\x97\xa5=%ld \xcc\x81=%ld \xf0\x9f\x9a\x80=%ld",
            tui_utf8 ? "UTF-8" : "Latin-1", tui_cterm ? "CTerm" : "console",
            (long)tui_charwidth('e'), (long)tui_charwidth(0x65E5), (long)tui_charwidth(0x301),
            (long)tui_charwidth(0x1F680));
    tui_pututf8(0, 0, buf, -1, 15, TC_DEFAULT, TA_BOLD);

    label(2, "PutUTF8 wide, then |");
    n = tui_pututf8(24, 2, jp, -1, 14, TC_DEFAULT, 0);
    tui_putc(24 + n, 2, '|', 10, TC_DEFAULT, 0);
    label(3, "  (should be: 3 wide, | at col 30)");

    label(4, "PutUTF8 accents");
    tui_pututf8(24, 4, "e\xcc\x81te\xcc\x81 G\xc3\xb6teborg|", -1, 14, TC_DEFAULT, 0);
    label(5, "  (should be: ete with acutes, Goteborg with o umlaut)");

    label(6, "PutUTF8 emoji, then |");
    n = tui_pututf8(24, 6, "\xf0\x9f\x98\x80\xf0\x9f\x9a\x80 ok", -1, 14, TC_DEFAULT, 0);
    tui_putc(24 + n, 6, '|', 10, TC_DEFAULT, 0);
    label(7, "  (should be: 2 emoji, ok, |)");

    label(8, "old way: one col each");
    for (i = 0; i < 3; i++) tui_putc(24 + i, 8, jpc[i], 14, TC_DEFAULT, 0);
    tui_put(27, 8, "abc", 3, 14, TC_DEFAULT, 0);
    label(9, "  (should be: ???abc, as before 0.5)");

    label(10, "new way: widths");
    for (i = 0, x = 24; i < 3; i++) {
        tui_putc(x, 10, jpc[i], 14, TC_DEFAULT, 0);
        x += tui_charwidth(jpc[i]);
    }
    tui_put(x, 10, "abc", 3, 14, TC_DEFAULT, 0);
    label(11, "  (should be: 3 wide, abc)");

    label(12, "wide at the last col");
    tui_putc(tui_cols - 1, 12, 0x65E5, 9, TC_DEFAULT, 0);
    label(13, "  (should be: ? at the far right)");

    tui_flush();
    tui_wait(&e, secs * 1000000, 0, &got);
    /* overwrite half: the left half shows ? after the next flush */
    tui_put(25, 2, "x", 1, 9, TC_DEFAULT, 0);
    label(14, "after a key: the first wide one cut in half by x: ?x");
    tui_flush();
    tui_wait(&e, secs * 1000000, 0, &got);
    tui_close();
    return 0;
}
