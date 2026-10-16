/* fuzz.c - json.c under libFuzzer (make fuzz): any bytes, any flags.
 * Whatever is read must write, read back the same, walk, convert and
 * free without a sanitizer complaint. */
#include "../src/port.h"

static char out[1 << 16];

int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size)
{
    ULONG flags;
    JsonDoc *d, *d2;
    JsonNode *n, *r;
    char path[64];
    int i;
    LONG need, need2;
    if (size < 1) return 0;
    flags = data[0] & JPF_RELAXED;
    d = JsonParse((const char *)data + 1, (LONG)size - 1, flags, NULL);
    if (!d) return 0;
    r = JsonRoot(d);

    /* written two ways, read back, written again: the same */
    need = JsonWrite(r, NULL, 0, data[0] >> 4 & 3);
    if (need < (LONG)sizeof(out)) {
        JsonWrite(r, out, sizeof(out), data[0] >> 4 & 3);
        if (!(d2 = JsonParse(out, need, 0, NULL))) __builtin_trap();
        need2 = JsonWrite(JsonRoot(d2), NULL, 0, 0);
        if (need2 != JsonWrite(r, NULL, 0, 0)) __builtin_trap();
        JsonFree(d2);
    }

    /* walk everything with the calls, a path made from the input */
    for (n = r; n;) {
        char b[16];
        JsonLatin1(n, b, sizeof(b));
        JsonInt(n, 0);
        JsonScaled(n, data[0] % 10, 0);
        JsonBool(n, 0);
        JsonName(n);
        JsonAt(n, -1);
        if (JsonFirst(n)) {
            n = JsonFirst(n);
            continue;
        }
        while (n && !JsonNext(n)) n = JsonParent(n);
        if (n) n = JsonNext(n);
    }
    for (i = 0; i < (int)sizeof(path) - 1 && i + 1 < (int)size; i++) path[i] = data[size - 1 - i] ? data[size - 1 - i] : '.';
    path[i] = 0;
    JsonFind(r, path);

    /* building on it: move a member into a new array and back */
    if ((n = JsonFirst(r)) != NULL) {
        JsonNode *a = JsonNewArray(d);
        JsonRemove(n);
        JsonAdd(a, NULL, n);
        JsonAdd(r, "x", a);
        JsonWrite(r, NULL, 0, JWF_ASCII);
    }
    JsonFree(d);
    return 0;
}
