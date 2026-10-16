/* fuzzytest - fuzzy.library tried on the Amiga, through the library: the
 * calls checked, then (with an index) searches timed.
 *
 *   fuzzytest                   the checks (writes and reads T:fuzzytest.index)
 *   fuzzytest INDEX QUERY...    also: each QUERY searched in INDEX, timed
 *
 * Returns 0 when all passed, 10 otherwise (20 without the library). */
#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/fuzzy.h>

struct Library *FuzzyBase;
static int fails;

#define CHECK(c) do { if (!(c)) { printf("line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static LONG score(const char *pat, const char *text, ULONG flags)
{
    FuzzyPattern *p = FuzzyPrepare(pat, flags);
    LONG s = FuzzyScore(p, text, -1, NULL, 0);
    FuzzyFreePattern(p);
    return s;
}

static ULONG ticks(void)
{
    struct DateStamp d;
    DateStamp(&d);
    return (ULONG)d.ds_Minute * 3000 + d.ds_Tick;
}

int main(int argc, char **argv)
{
    static const char *const paths[] = { "Work:", "Work:DPaint", "Work:DPaint/DPaint",
                                         "Work:G\xf6teborg.txt", "Work:Music", "Work:Music/mod.foo" };
    FuzzyWriter *w;
    struct FuzzyIndex *x;
    struct FuzzyEntry e;
    struct FuzzyResults *r;
    UWORD pos[8];
    LONG err, i;
    char buf[256];

    if (!(FuzzyBase = OpenLibrary(FUZZYNAME, 0))) {
        printf("fuzzytest: needs fuzzy.library\n");
        return 20;
    }
    printf("fuzzy.library %d.%d\n", FuzzyBase->lib_Version, FuzzyBase->lib_Revision);

    /* matching */
    CHECK(score("csrc", "config/source.c", 0) > 0);
    CHECK(score("csrc", "config/sauce.c", 0) == 0);
    CHECK(score("goteborg", "Work:G\xf6teborg.txt", 0) > 0);
    CHECK(score("goteborg", "Work:G\xf6teborg.txt", FPF_ACCENTS) == 0);
    CHECK(score("WORK", "Work:x", FPF_CASE) == 0);
    CHECK(score("dpaint", "Work:Graphics/DPaint/DPaint", 0) > score("dpaint", "Work:Docs/dpaint-notes/a.txt", 0));
    {
        FuzzyPattern *p = FuzzyPrepare("csrc", 0);
        CHECK(FuzzyScore(p, "config/source.c", -1, pos, 8) > 0);
        CHECK(pos[0] == 0 && pos[1] == 7 && pos[2] == 10 && pos[3] == 11);
        FuzzyFreePattern(p);
    }

    /* the default place, and the error texts */
    FuzzyIndexPath(buf, sizeof(buf));
    printf("index place: %s\n", buf);
    CHECK(!strcmp(FuzzyErrorText(FERR_NOTINDEX), "not a FuzzyFind index"));

    /* an index written and read back */
    w = FuzzyNewIndex("T:fuzzytest.index", &err);
    CHECK(w != NULL);
    if (w) {
        CHECK(FuzzyAddVolume(w, "Work", 1234) == FERR_OK);
        for (i = 0; i < 6; i++) {
            e.Path = paths[i];
            e.PathLen = -1;
            e.Type = (i == 0 || i == 1 || i == 4) ? FET_DRAWER : FET_FILE;
            e.Prot = 0;
            e.Size = i * 100;
            e.Date = 1000 + i;
            CHECK(FuzzyAddEntry(w, &e));
        }
        CHECK(FuzzyEndIndex(w, TRUE) == FERR_OK);
    }
    x = FuzzyOpenIndex("T:fuzzytest.index", &err);
    CHECK(x != NULL);
    if (x) {
        ULONG d;
        CHECK(x->Count == 6 && x->Volumes == 1);
        CHECK(!strcmp(FuzzyVolume(x, 0, &d), "Work") && d == 1234);
        for (i = 0; FuzzyNextEntry(x, &e); i++)
            CHECK(!strcmp(e.Path, paths[i]) && e.Date == 1000UL + i);
        CHECK(i == 6);
        r = FuzzySearch(x, "dpaint", 0, 5);
        CHECK(r && r->Count == 2 && !strcmp(r->Hits[0].Path, "Work:DPaint"));
        FuzzyFreeResults(r);
        r = FuzzySearch(x, "dpaint", FSF_FILES, 5);
        CHECK(r && r->Count == 1 && !strcmp(r->Hits[0].Path, "Work:DPaint/DPaint"));
        FuzzyFreeResults(r);
        FuzzyCloseIndex(x);
    }
    DeleteFile("T:fuzzytest.index");
    CHECK(FuzzyOpenIndex("T:nothing.index", &err) == NULL && err == FERR_NOFILE);

    /* searches timed */
    if (argc > 2 && (x = FuzzyOpenIndex(argv[1], &err))) {
        ULONG t0;
        printf("%s: %ld entries\n", argv[1], (long)x->Count);
        t0 = ticks();
        for (i = 0; FuzzyNextEntry(x, &e); i++)
            ;
        printf("  reading every entry: %lu ms\n", (unsigned long)(ticks() - t0) * 20);
        for (i = 2; i < argc; i++) {
            LONG j;
            t0 = ticks();
            r = FuzzySearch(x, argv[i], 0, 5);
            printf("  \"%s\": %ld match, %lu ms\n", argv[i], (long)r->Matched,
                   (unsigned long)(ticks() - t0) * 20);
            for (j = 0; j < r->Count; j++) printf("    %s\n", r->Hits[j].Path);
            FuzzyFreeResults(r);
        }
        FuzzyCloseIndex(x);
    } else if (argc > 2)
        printf("%s: %s\n", argv[1], FuzzyErrorText(err));

    CloseLibrary(FuzzyBase);
    if (fails) {
        printf("fuzzytest: %d failed\n", fails);
        return 10;
    }
    printf("fuzzytest: all passed\n");
    return 0;
}
