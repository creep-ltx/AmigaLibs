/* first.c - the programmer's guide's first program: UTF-8 text shown on
 * an Amiga (Latin-1) console, measured, and searched. */
#include <stdio.h>
#include <proto/exec.h>
#include <proto/utf8.h>

struct Library *Utf8Base;

int main(void)
{
    static const char *lines[] = {
        "G\xc3\xb6teborg \xe2\x80\x93 \xe2\x80\x9cLittle London\xe2\x80\x9d, 5 \xe2\x82\xac",
        "\xc5\x81\xc3\xb3" "d\xc5\xba, Krak\xc3\xb3w, Gda\xc5\x84sk",
        "\xe6\x9d\xb1\xe4\xba\xac means Tokyo \xf0\x9f\x97\xbc",
    };
    char shown[80];
    int i;

    if (!(Utf8Base = OpenLibrary(UTF8NAME, 0))) {
        printf("needs utf8.library\n");
        return 20;
    }
    for (i = 0; i < 3; i++) {
        Utf8ToCharset(lines[i], -1, shown, sizeof(shown), U8CS_LATIN1, 0);
        printf("%-34s %2ld characters, %2ld columns\n", shown,
               (long)Utf8Length(lines[i], -1), (long)Utf8Width(lines[i], -1));
    }
    if (Utf8Find(lines[0], -1, "goteborg", -1, U8F_NOCASE | U8F_NOACCENT, NULL) >= 0)
        printf("\"goteborg\" found without case or accents\n");
    CloseLibrary(Utf8Base);
    return 0;
}
