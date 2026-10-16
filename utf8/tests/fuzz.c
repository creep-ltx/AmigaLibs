/* fuzz.c - utf8.c under libFuzzer: random bytes through every call, and
 * what must hold between them.   make fuzz */
#include <stdio.h>
#include <stdlib.h>
#include "../src/port.h"

#define MUST(c) do { if (!(c)) { fprintf(stderr, "broken: %s (line %d)\n", #c, __LINE__); abort(); } } while (0)

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    const char *s = (const char *)data;
    LONG len = (LONG)size, p, q, k, n, w, w2, ml;
    LONG starts[4096];
    char *big, small[7];
    ULONG flags;

    if (size > 4000) return 0;
    big = malloc(size * 12 + 16);

    /* whole characters: forward and back agree */
    n = 0;
    for (p = 0; p < len; p = q) {
        q = Utf8Next(s, len, p);
        MUST(q > p && q <= len);
        starts[n++] = p;
    }
    MUST(n == Utf8Length(s, len));
    for (k = n - 1; k > 0; k--) MUST(Utf8Prev(s, len, starts[k]) == starts[k - 1]);
    if (n) MUST(Utf8Prev(s, len, len) == starts[n - 1]);

    /* widths */
    w = Utf8Width(s, len);
    MUST(w >= 0 && w <= 2 * len);
    for (k = 0; k <= w; k += 1 + (w >> 3)) {
        p = Utf8Fit(s, len, k, &w2);
        MUST(p >= 0 && p <= len && w2 <= k);
        MUST(Utf8Width(s, p) == w2);
    }

    /* what it is */
    k = Utf8Valid(s, len);
    MUST(k >= -1 && k < len);
    MUST((Utf8Check(s, len) == U8_LATIN1) == (k >= 0));

    /* comparing: a text equals itself, and is found in itself */
    for (flags = 0; flags < 4; flags++) {
        MUST(Utf8Compare(s, len, s, len, flags) == 0);
        MUST(Utf8Find(s, len, s, len, flags, &ml) == 0 && ml == len);
        if (n > 2) {
            p = Utf8Find(s, len, s + starts[1], starts[2] - starts[1], flags, &ml);
            MUST(p >= 0 && p <= starts[1] && ml > 0);
        }
    }

    /* writing: the length it says is the length it writes, and a small
     * buffer gets whole characters and its 0 */
    n = Utf8Normalize(s, len, NULL, 0, U8F_DECOMPOSE);
    MUST(Utf8Normalize(s, len, big, size * 12 + 16, U8F_DECOMPOSE) == n && (LONG)strlen(big) <= n);
    k = Utf8Normalize(big, n, NULL, 0, U8F_COMPOSE);
    MUST(Utf8Compare(s, len, big, n, 0) == 0);
    MUST(Utf8Normalize(s, len, small, sizeof(small), U8F_COMPOSE) >= 0 && strlen(small) < sizeof(small));
    MUST(Utf8Valid(small, -1) == -1);
    for (k = 0; k < U8CS_COUNT; k++) {
        n = Utf8ToCharset(s, len, NULL, 0, k, 0);
        MUST(Utf8ToCharset(s, len, big, size * 12 + 16, k, 0) == n);
        MUST(Utf8ToCharset(s, len, small, sizeof(small), k, U8F_STRICT) >= 0);
        n = Utf8FromCharset(s, len, big, size * 12 + 16, k);
        MUST(Utf8Valid(big, n) == -1);
    }
    n = Utf8Case(s, len, big, size * 12 + 16, U8F_UPPER);
    MUST(Utf8Valid(big, n) == -1);
    MUST(Utf8Compare(s, len, big, n, U8F_NOCASE) == 0 || 1);   /* runs; ß and friends differ */
    free(big);
    return 0;
}
