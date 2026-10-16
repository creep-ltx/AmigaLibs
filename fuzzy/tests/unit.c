/* unit.c - fuzzy.library's core on the PC: matching, ranking, the index
 * written and read back, searching. Built with the address and
 * undefined-behaviour sanitizers (make pctest). */
#include "../src/port.h"

static int fails;

#define CHECK(c) do { if (!(c)) { printf("%s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static LONG score(const char *pat, const char *text, ULONG flags)
{
    FuzzyPattern *p = FuzzyPrepare(pat, flags);
    LONG s = FuzzyScore(p, text, -1, NULL, 0);
    FuzzyFreePattern(p);
    return s;
}

/* a must rank above b for pat */
static void above(const char *pat, const char *a, const char *b)
{
    LONG sa = score(pat, a, 0), sb = score(pat, b, 0);
    if (!(sa > sb)) {
        printf("\"%s\": \"%s\" (%d) should beat \"%s\" (%d)\n", pat, a, (int)sa, b, (int)sb);
        fails++;
    }
}

static void matching(void)
{
    UWORD pos[16];
    FuzzyPattern *p;
    LONG s;

    CHECK(score("csrc", "config/source.c", 0) > 0);
    CHECK(score("csrc", "config/sauce.c", 0) == 0);            /* no r */
    CHECK(score("", "anything", 0) == 1);                      /* nothing typed: all */
    CHECK(score("   ", "anything", 0) == 1);
    CHECK(score("x", "", 0) == 0);
    CHECK(score("WORK", "Work:Music", 0) > 0);                 /* case */
    CHECK(score("WORK", "Work:Music", FPF_CASE) == 0);
    CHECK(score("goteborg", "Work:G\xf6teborg.txt", 0) > 0);    /* accents */
    CHECK(score("goteborg", "Work:G\xf6teborg.txt", FPF_ACCENTS) == 0);
    CHECK(score("g\xf6teborg", "Work:GOTEBORG", 0) > 0);        /* both ways */
    CHECK(score("\xe5sa", "\xc5SA", 0) > 0);
    CHECK(score("\xe5sa", "\xc5SA", FPF_CASE) == 0);
    CHECK(score("mod music", "Work:Music/mod.foo", 0) > 0);    /* words, any order */
    CHECK(score("mod nothing", "Work:Music/mod.foo", 0) == 0);
    CHECK(score("src", "Work:s/r/c", FPF_PLAIN) == 0);         /* plain: one piece */
    CHECK(score("src", "Work:x/src.c", FPF_PLAIN) > 0);

    /* the ranking fzf users expect */
    above("dpaint", "Work:Graphics/DPaint/DPaint", "Work:Docs/dpaint-notes/a.txt");
    above("dp", "Work:DPaint", "Work:Graphics/cdplayer");
    above("ff", "Work:FuzzyFind", "Work:offline");          /* word starts */
    above("ff", "Work:fuzzy_find", "Work:offline");
    above("music", "Work:Music", "Work:M/u/s/i/c");          /* in a row */
    above("readme", "Work:Projects/README", "Work:Projects/README/old/x");  /* shorter */
    above("readme", "Work:x/README", "Work:README-dir/x");    /* in the name */
    above("ami", "AmigaOS:C/Avail", "Work:Ahiru/m/i");

    /* where the letters were */
    p = FuzzyPrepare("csrc", 0);
    memset(pos, 0xFF, sizeof(pos));
    s = FuzzyScore(p, "config/source.c", -1, pos, 16);
    CHECK(s > 0);
    CHECK(pos[0] == 0 && pos[1] == 7 && pos[2] == 10 && pos[3] == 11);
    FuzzyFreePattern(p);
    p = FuzzyPrepare("abc", 0);
    memset(pos, 0xFF, sizeof(pos));
    s = FuzzyScore(p, "abc", -1, pos, 2);                      /* no more than asked */
    CHECK(s > 0 && pos[2] == 0xFFFF);
    FuzzyFreePattern(p);

    CHECK(FuzzyScore(NULL, "x", -1, NULL, 0) == 0);
    CHECK(FuzzyPrepare(NULL, 0) == NULL);
    FuzzyFreePattern(NULL);
}

static void index_rw(void)
{
    static const char *const paths[] = {
        "Work:", "Work:Music", "Work:Music/mod.foo", "Work:Music/mod.bar", "Work:DPaint",
        "Work:DPaint/DPaint", "Work:G\xf6teborg.txt", "Work:Docs", "Work:Docs/readme",
    };
    static const UBYTE types[] = { 1, 1, 0, 0, 1, 0, 0, 1, 0 };
    FuzzyWriter *w;
    struct FuzzyIndex *x;
    struct FuzzyEntry e;
    struct FuzzyResults *r;
    LONG err, i;
    ULONG date;
    FILE *f;
    char big[MAXPATH + 10];

    w = FuzzyNewIndex("/tmp/fuzzyunit.index", &err);
    CHECK(w != NULL && err == FERR_OK);
    CHECK(FuzzyAddVolume(w, "Work", 123456) == FERR_OK);
    CHECK(FuzzyAddVolume(w, "AmigaOS", 7) == FERR_OK);
    for (i = 0; i < 9; i++) {
        e.Path = paths[i];
        e.PathLen = -1;
        e.Type = types[i];
        e.Prot = (UBYTE)i;
        e.Size = types[i] ? 0 : 1000 * i + 70000;
        e.Date = 1000000000 + i;
        CHECK(FuzzyAddEntry(w, &e));
    }
    memset(big, 'x', sizeof(big) - 1);
    big[sizeof(big) - 1] = 0;
    e.Path = big;
    CHECK(!FuzzyAddEntry(w, &e));                               /* too long: refused */
    CHECK(FuzzyAddVolume(w, "Late", 0) == FERR_ORDER);
    CHECK(FuzzyEndIndex(w, TRUE) == FERR_OK);
    f = fopen("/tmp/fuzzyunit.index.new", "rb");
    CHECK(f == NULL);                                           /* renamed into place */
    if (f) fclose(f);

    x = FuzzyOpenIndex("/tmp/fuzzyunit.index", &err);
    CHECK(x != NULL && err == FERR_OK);
    if (!x) return;
    CHECK(x->Count == 9 && x->Volumes == 2);
    CHECK(!strcmp(FuzzyVolume(x, 1, &date), "AmigaOS") && date == 7);
    CHECK(FuzzyVolume(x, 2, NULL) == NULL);
    for (i = 0; FuzzyNextEntry(x, &e); i++) {
        CHECK(!strcmp(e.Path, paths[i]));
        CHECK(e.Type == types[i] && e.Prot == i && e.Date == 1000000000UL + i);
        CHECK(e.Size == (types[i] ? 0UL : 1000UL * i + 70000));
    }
    CHECK(i == 9);
    FuzzyRewind(x);
    CHECK(FuzzyNextEntry(x, &e) && !strcmp(e.Path, "Work:"));

    r = FuzzySearch(x, "dpaint", 0, 10);
    CHECK(r && r->Count == 2 && r->Matched == 2);
    if (r && r->Count) CHECK(!strcmp(r->Hits[0].Path, "Work:DPaint"));    /* shorter first */
    FuzzyFreeResults(r);
    r = FuzzySearch(x, "dpaint", FSF_FILES, 10);
    CHECK(r && r->Count == 1 && !strcmp(r->Hits[0].Path, "Work:DPaint/DPaint") && r->Hits[0].NameOffset == 12);
    FuzzyFreeResults(r);
    r = FuzzySearch(x, "mod", FSF_NAME, 1);                     /* only the best one kept */
    CHECK(r && r->Count == 1 && r->Matched == 2);
    FuzzyFreeResults(r);
    r = FuzzySearch(x, "goteborg", 0, 10);
    CHECK(r && r->Count == 1);
    FuzzyFreeResults(r);
    r = FuzzySearch(x, "", FSF_DRAWERS, 100);                   /* everything of a kind */
    CHECK(r && r->Count == 4);
    FuzzyFreeResults(r);
    FuzzyCloseIndex(x);

    /* refusals */
    CHECK(FuzzyOpenIndex("/tmp/nothing-here.index", &err) == NULL && err == FERR_NOFILE);
    f = fopen("/tmp/fuzzyunit.bad", "wb");
    fputs("not an index at all", f);
    fclose(f);
    CHECK(FuzzyOpenIndex("/tmp/fuzzyunit.bad", &err) == NULL && err == FERR_NOTINDEX);
    f = fopen("/tmp/fuzzyunit.bad", "wb");
    fwrite("FFIX\0\x09", 1, 6, f);
    fclose(f);
    CHECK(FuzzyOpenIndex("/tmp/fuzzyunit.bad", &err) == NULL && err == FERR_VERSION);
    remove("/tmp/fuzzyunit.bad");

    /* an index cut short reads what is whole, then stops */
    f = fopen("/tmp/fuzzyunit.index", "rb");
    {
        char b[4096];
        size_t n = fread(b, 1, sizeof(b), f);
        fclose(f);
        f = fopen("/tmp/fuzzyunit.cut", "wb");
        fwrite(b, 1, n - 20, f);
        fclose(f);
    }
    x = FuzzyOpenIndex("/tmp/fuzzyunit.cut", &err);
    CHECK(x != NULL);
    for (i = 0; x && FuzzyNextEntry(x, &e); i++)
        ;
    CHECK(i > 0 && i < 9);
    FuzzyCloseIndex(x);
    remove("/tmp/fuzzyunit.cut");

    /* abandoned: nothing left behind */
    w = FuzzyNewIndex("/tmp/fuzzyunit2.index", &err);
    CHECK(FuzzyEndIndex(w, FALSE) == FERR_OK);
    CHECK(fopen("/tmp/fuzzyunit2.index.new", "rb") == NULL && fopen("/tmp/fuzzyunit2.index", "rb") == NULL);
    remove("/tmp/fuzzyunit.index");
}

/* a big index: 100,000 entries in a deep tree, searched */
static void big(void)
{
    FuzzyWriter *w = FuzzyNewIndex("/tmp/fuzzybig.index", NULL);
    struct FuzzyIndex *x;
    struct FuzzyResults *r;
    struct FuzzyEntry e;
    char path[200];
    LONG i;
    FILE *f;
    long size;
    for (i = 0; i < 100000; i++) {
        sprintf(path, "Work:Projects/p%03ld/src/module%02ld/file%05ld.c", (long)(i / 1000), (long)(i / 50 % 20), (long)i);
        e.Path = path;
        e.PathLen = -1;
        e.Type = FET_FILE;
        e.Prot = 0;
        e.Size = (ULONG)i;
        e.Date = 0;
        FuzzyAddEntry(w, &e);
    }
    CHECK(FuzzyEndIndex(w, TRUE) == FERR_OK);
    f = fopen("/tmp/fuzzybig.index", "rb");
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fclose(f);
    printf("100,000 paths: %ld KB on disk\n", size / 1024);
    x = FuzzyOpenIndex("/tmp/fuzzybig.index", NULL);
    r = FuzzySearch(x, "p042 file42001", 0, 5);
    CHECK(r && r->Count >= 1 && strstr(r->Hits[0].Path, "file42001"));
    FuzzyFreeResults(r);
    FuzzyCloseIndex(x);
    remove("/tmp/fuzzybig.index");
}

int main(void)
{
    matching();
    index_rw();
    big();
    if (fails) {
        printf("unit: %d failed\n", fails);
        return 1;
    }
    printf("unit: all passed\n");
    return 0;
}
