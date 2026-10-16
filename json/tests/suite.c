/* suite.c - json.c against JSONTestSuite (github.com/nst/JSONTestSuite,
 * test_parsing/): y_ files must be read, n_ files refused, i_ files are
 * the reader's choice (listed). Everything read is written, read again
 * and written again: both writings must be the same, compact and pretty,
 * and the ASCII-only writing must read back to the same.
 *
 *   suite DIR       (make pctest SUITE=DIR) */
#include "../src/port.h"
#include <dirent.h>

static char *slurp(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    char *b;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *len = ftell(f);
    fseek(f, 0, SEEK_SET);
    b = malloc(*len + 1);
    if (fread(b, 1, *len, f) != (size_t)*len) *len = 0;
    fclose(f);
    return b;
}

static char *dump(JsonNode *n, ULONG flags)
{
    LONG need = JsonWrite(n, NULL, 0, flags);
    char *b = malloc(need + 1);
    if (JsonWrite(n, b, need + 1, flags) != need) {
        printf("JsonWrite gave two lengths\n");
        exit(1);
    }
    return b;
}

/* written, read again, written again: the same */
static int roundtrip(const char *name, JsonNode *root)
{
    static const ULONG fl[] = { 0, JWF_PRETTY, JWF_ASCII, JWF_ASCII | JWF_PRETTY };
    char *one = dump(root, 0);
    int i, ok = 1;
    for (i = 0; i < 4; i++) {
        char *t = dump(root, fl[i]), *two;
        JsonDoc *d = JsonParse(t, -1, 0, NULL);
        if (!d) {
            printf("%s: its own writing (flags %lu) does not read back\n", name, (unsigned long)fl[i]);
            ok = 0;
        } else {
            two = dump(JsonRoot(d), 0);
            if (strcmp(one, two)) {
                printf("%s: writing (flags %lu) changed it\n", name, (unsigned long)fl[i]);
                ok = 0;
            }
            free(two);
            JsonFree(d);
        }
        free(t);
    }
    free(one);
    return ok;
}

int main(int argc, char **argv)
{
    DIR *dir;
    struct dirent *de;
    int pass = 0, bad = 0, iyes = 0, ino = 0;
    char path[1024];
    if (argc < 2 || !(dir = opendir(argv[1]))) {
        printf("usage: suite JSONTestSuite/test_parsing\n");
        return 2;
    }
    while ((de = readdir(dir)) != NULL) {
        const char *n = de->d_name;
        long len;
        char *text;
        JsonDoc *d;
        if (n[0] != 'y' && n[0] != 'n' && n[0] != 'i') continue;
        snprintf(path, sizeof(path), "%s/%s", argv[1], n);
        if (!(text = slurp(path, &len))) continue;
        d = JsonParse(text, len, 0, NULL);
        if (n[0] == 'y' && !d) { printf("refused, should read: %s\n", n); bad++; }
        else if (n[0] == 'n' && d) { printf("read, should refuse: %s\n", n); bad++; }
        else if (n[0] == 'i') { printf("%s: %s\n", d ? "reads  " : "refuses", n); d ? iyes++ : ino++; }
        else pass++;
        if (d) {
            if (!roundtrip(n, JsonRoot(d))) bad++;
            JsonFree(d);
        }
        /* relaxed reading must never crash either */
        if ((d = JsonParse(text, len, JPF_RELAXED, NULL)) != NULL) {
            if (!roundtrip(n, JsonRoot(d))) bad++;
            JsonFree(d);
        }
        free(text);
    }
    closedir(dir);
    printf("suite: %d right, %d wrong; implementation-defined: %d read, %d refused\n", pass, bad, iyes, ino);
    return bad != 0;
}
