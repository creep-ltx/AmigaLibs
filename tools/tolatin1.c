/* tolatin1: UTF-8 on stdin to Latin-1 on stdout, with utf8.library's own
 * Utf8ToCharset (the library's C source, built for the PC): what Latin-1
 * lacks becomes its nearest, other scripts spelt in Latin letters, as an
 * Amiga shows it. For tools/mkdocs.py.
 *   cc -I../utf8/tests/pcinc -o tolatin1 tolatin1.c ../utf8/src/utf8.c ../utf8/src/tables.c */
#include <stdio.h>
#include <stdlib.h>
#include "../utf8/src/port.h"

int main(void)
{
    size_t n = 0, cap = 1 << 16, r;
    char *in = malloc(cap), *out;
    LONG len;
    while ((r = fread(in + n, 1, cap - n, stdin)) > 0)
        if ((n += r) == cap) in = realloc(in, cap *= 2);
    out = malloc(n * 4 + 16);
    len = Utf8ToCharset((CONST_STRPTR)in, (LONG)n, (STRPTR)out, (LONG)(n * 4 + 16), U8CS_LATIN1, 0);
    if (len < 0) return 1;
    fwrite(out, 1, (size_t)len, stdout);
    return 0;
}
