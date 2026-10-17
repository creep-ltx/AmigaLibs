/* rank.c - on the PC: an index from a list of paths (one a line), then
 * the best matches for each query, to set beside fzf's (make rank).
 *   rank LIST QUERY... */
#include "../src/port.h"

int main(int argc, char **argv)
{
    FILE *f;
    char line[2048];
    FuzzyWriter *w;
    struct FuzzyIndex *x;
    LONG i, n = 0;
    if (argc < 3 || !(f = fopen(argv[1], "rb"))) return 2;
    w = FuzzyNewIndex("/tmp/rank.index", NULL);
    while (fgets(line, sizeof(line), f)) {
        struct FuzzyEntry e;
        size_t k = strcspn(line, "\r\n");
        line[k] = 0;
        if (!k) continue;
        e.Path = line;
        e.PathLen = (LONG)k;
        e.Type = FET_FILE;
        e.Prot = 0;
        e.Size = 0;
        e.Date = 0;
        if (FuzzyAddEntry(w, &e)) n++;
    }
    fclose(f);
    FuzzyEndIndex(w, TRUE);
    f = fopen("/tmp/rank.index", "rb");
    fseek(f, 0, SEEK_END);
    printf("%ld paths, index %ld KB\n", (long)n, ftell(f) / 1024);
    fclose(f);
    x = FuzzyOpenIndex("/tmp/rank.index", NULL);
    {   /* the name offsets the reader keeps must equal a full scan's */
        struct FuzzyEntry e;
        LONG bad = 0;
        while (FuzzyNextEntry(x, &e))
            if (e.NameOffset != fuzzy_nameoffset((const UBYTE *)e.Path, e.PathLen)) bad++;
        printf("name offsets: %ld wrong\n", (long)bad);
        FuzzyRewind(x);
    }
    for (i = 2; i < argc; i++) {
        struct FuzzyResults *r = FuzzySearch(x, argv[i], 0, 5);
        LONG j;
        printf("== %s (%ld match)\n", argv[i], (long)r->Matched);
        for (j = 0; j < r->Count; j++) printf("  %4ld %s\n", (long)r->Hits[j].Score, r->Hits[j].Path);
        FuzzyFreeResults(r);
    }
    FuzzyCloseIndex(x);
    remove("/tmp/rank.index");
    return 0;
}
