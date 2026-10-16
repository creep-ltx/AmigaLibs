/* mkindex.c - on the PC: an index from a list of paths, one a line (for
 * trying the library on the Amiga with a real machine's paths).
 *   mkindex LIST INDEX */
#include "../src/port.h"

int main(int argc, char **argv)
{
    FILE *f;
    char line[2048];
    FuzzyWriter *w;
    LONG n = 0;
    if (argc < 3 || !(f = fopen(argv[1], "rb")) || !(w = FuzzyNewIndex(argv[2], NULL))) return 2;
    while (fgets(line, sizeof(line), f)) {
        struct FuzzyEntry e;
        size_t k = strcspn(line, "\r\n");
        line[k] = 0;
        if (!k) continue;
        e.Path = line;
        e.PathLen = (LONG)k;
        e.Type = strchr(line + k - 5 > line ? line + k - 5 : line, '.') ? FET_FILE : FET_DRAWER;
        e.Prot = 0;
        e.Size = 0;
        e.Date = 0;
        if (FuzzyAddEntry(w, &e)) n++;
    }
    fclose(f);
    printf("%ld paths\n", (long)n);
    return FuzzyEndIndex(w, TRUE) != FERR_OK;
}
