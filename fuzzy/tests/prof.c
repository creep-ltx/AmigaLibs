/* prof.c - where a search's time goes, on the Amiga: the core built in
 * (no library), each stage timed. For finding the hot spots.
 *   prof INDEX PATTERN */
#include <stdio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "../src/port.h"

LONG prof_masked, prof_scored, prof_matched;
static ULONG ticks(void) { struct DateStamp d; DateStamp(&d); return (ULONG)d.ds_Minute * 3000 + d.ds_Tick; }

int main(int argc, char **argv)
{
    struct FuzzyIndex *x;
    struct FuzzyEntry e;
    struct FuzzyResults *r;
    FuzzyPattern *p;
    ULONG t0;
    LONG n = 0, sc = 0;
    if (argc < 3 || !(x = FuzzyOpenIndex(argv[1], NULL))) return 10;
    t0 = ticks();
    while (FuzzyNextEntry(x, &e)) n++;
    printf("read %ld entries (no fingerprint): %lu ms\n", (long)n, (ticks() - t0) * 20);
    p = FuzzyPrepare(argv[2], 0);
    FuzzyRewind(x);
    t0 = ticks();
    while (FuzzyNextEntry(x, &e)) sc += FuzzyScore(p, e.Path, e.PathLen, NULL, 0) > 0;
    printf("read + score every entry (no fingerprint): %lu ms, %ld match\n", (ticks() - t0) * 20, (long)sc);
    t0 = ticks();
    r = FuzzySearch(x, argv[2], 0, 5);
    printf("FuzzySearch: %lu ms; past the fingerprint %ld, scored %ld, matched %ld\n",
           (ticks() - t0) * 20, (long)prof_masked, (long)prof_scored, (long)r->Matched);
    FuzzyFreeResults(r);
    FuzzyFreePattern(p);
    FuzzyCloseIndex(x);
    return 0;
}
