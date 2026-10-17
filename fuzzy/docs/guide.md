# fuzzy.library programmer's guide

fuzzy.library does two things:

- **Fuzzy matching**, as fzf does it: `csrc` finds `config/source.c`, and the best matches come
  first. For file pickers, command history, launchers, "jump to" boxes.
- **The FuzzyFind index**: a catalogue of every file and drawer on the chosen disks, with size,
  date and type. FuzzyFind's `updatedb` writes it; any program can read and search it, without
  walking the disks.

It works from C (and any language that can call a library) and from **ARexx**.

This guide explains how to use it. Every call is described in full in the
[reference](reference.md). Working programs: [tests/fuzzytest.c](../tests/fuzzytest.c) (C),
[tests/fuzzytest.rexx](../tests/fuzzytest.rexx) (ARexx), and FuzzyFind's `updatedb`, `locate` and
`fzf`.

- [What you need](#what-you-need)
- [The first program](#the-first-program)
- [Matching](#matching)
- [How the ranking works](#how-the-ranking-works)
- [The index: reading and searching](#the-index-reading-and-searching)
- [The index: writing](#the-index-writing)
- [The index format](#the-index-format)
- [Speed](#speed)
- [ARexx](#arexx)
- [Versions](#versions)
- [Rules and traps](#rules-and-traps)

## What you need

- AmigaOS 2.04 or newer, any 680x0.
- `LIBS:fuzzy.library`: the build for the processor (`build/000`, `020` or `060`, see the
  README). No floating point, so no FPU build.
- The headers in `include/`: `libraries/fuzzy.h`, `proto/fuzzy.h`, `inline/fuzzy.h` (gcc) and
  `clib/fuzzy_protos.h`. `fuzzy.sfd` has the calls and registers for other languages.
- For the index: one made by FuzzyFind's `updatedb` (or by your own program, below).

## The first program

The five best matches for a word, from the index:

```c
#include <stdio.h>
#include <proto/exec.h>
#include <proto/fuzzy.h>

struct Library *FuzzyBase;

int main(int argc, char **argv)
{
    struct FuzzyIndex *index;
    struct FuzzyResults *r;
    LONG err, i;

    if (argc < 2) {
        printf("usage: best WORD\n");
        return 10;
    }
    if (!(FuzzyBase = OpenLibrary(FUZZYNAME, 0))) {
        printf("needs fuzzy.library\n");
        return 20;
    }
    if (!(index = FuzzyOpenIndex(NULL, &err))) {        /* NULL: the usual place */
        printf("no index: %s\n", FuzzyErrorText(err));
        CloseLibrary(FuzzyBase);
        return 10;
    }
    if ((r = FuzzySearch(index, argv[1], 0, 5))) {     /* the 5 best */
        printf("%ld of %ld entries match\n", (long)r->Matched, (long)index->Count);
        for (i = 0; i < r->Count; i++)
            printf("%5ld  %s\n", (long)r->Hits[i].Score, r->Hits[i].Path);
        FuzzyFreeResults(r);
    }
    FuzzyCloseIndex(index);
    CloseLibrary(FuzzyBase);
    return 0;
}
```

`best multiview` on a Workbench 3.2.1 disk prints (run 2026-10-17):

```
3 of 1575 entries match
  250  Workbench:Utilities/MultiView
  250  Workbench:Utilities/MultiView.info
  250  Workbench:Locale/Help/english/Sys/Utilities/multiview.help
```

Equal scores go shortest path first.

Build with bebbo's gcc: `m68k-amigaos-gcc -Os -noixemul -Wno-pointer-sign -I<fuzzy>/include
best.c -o best`.

## Matching

```c
FuzzyPattern *p = FuzzyPrepare("csrc", 0);              /* once */
LONG s = FuzzyScore(p, "config/source.c", -1, NULL, 0); /* for each text: 0 = no match */
FuzzyFreePattern(p);
```

- **Prepare once, score many times.** `FuzzyPrepare` folds the pattern and builds its tables;
  `FuzzyScore` is then cheap. A higher score is a better match; 0 is none. Sort by it.
- **The letters in order, with gaps:** `csrc` matches `config/source.c`, not `config/sauce.c`.
- **Several words** (a space between) must all match, in any order: `mod music` finds
  `Work:Music/mod.foo`. An empty pattern matches everything with score 1.
- **Case and accents do not matter** by default: `goteborg` finds `Göteborg`, `WORK` finds
  `Work:`.
- **Where the letters were**, for highlighting: pass a `UWORD` array and its size; it gets the
  byte position of each matched letter (in pattern order, word after word).

```c
UWORD pos[32];
if (FuzzyScore(p, text, -1, pos, 32))
    ...  /* pos[0] is where the first letter of the pattern was found */
```

| Flag for `FuzzyPrepare` | Means |
|---|---|
| `FPF_CASE` | case matters |
| `FPF_ACCENTS` | accents matter (`o` no longer finds `ö`) |
| `FPF_PLAIN` | each word as one piece of text, no gaps (`src` no longer matches `s/r/c`) |
| `FPF_TEXT` | the texts are not paths (history, commands): no file-name bonus, and a word after a space counts more |

Text is Latin-1, as Amiga file names are. Texts longer than 65535 bytes are scored on their
first 65535.

## How the ranking works

It is fzf's ranking (its "v1" way and its numbers), with its **path scheme** by default:

1. Find the pattern's letters in order, left to right; then from the last one backwards to the
   shortest window that holds them all.
2. Score that window: 16 a letter; more for a letter at the **start of a word** (after `/ : _ -
   . space`, a capital after a small letter, a digit after a letter), more again for letters **in a
   row**; a small penalty for each gap. The first letter's bonus counts double.
3. **The file name wins:** if the letters fit in the name (after the last `/` or `:`) that window
   is tried too, and a window wholly in the name gets a bonus. So `dpaint` puts
   `Graphics/DPaint/DPaint` above `Docs/dpaint-notes/a.txt`.
4. Ties (in `FuzzySearch`): the shorter path first, then the one that came first.

`:` counts as a path separator, as on the Amiga. On the user's A1200's 21,028 paths the top five
for `dpaint`, `startup`, `ahiru`, `mod` and `readme` were the same as fzf 0.74's, in the same
order (checked 2026-10-17).

## The index: reading and searching

```c
struct FuzzyIndex *x = FuzzyOpenIndex(NULL, &err);   /* NULL: FuzzyIndexPath()'s place */
```

`x->Count` is how many entries it has, `x->Created` when it was made (seconds since 1978-01-01, as
AmigaDOS counts), `x->Volumes` how many volumes it covers (`FuzzyVolume(x, n, &date)` gives each
name).

**Every entry, in order:**

```c
struct FuzzyEntry e;
while (FuzzyNextEntry(x, &e))
    printf("%s %s %lu\n", e.Type == FET_DRAWER ? "D" : "F", e.Path, e.Size);
```

| Field | |
|---|---|
| `Path`, `PathLen` | `"Work:Music/mod.foo"`, valid until the next `FuzzyNextEntry` |
| `NameOffset` | where the name starts in `Path` |
| `Type` | `FET_FILE`, `FET_DRAWER`, `FET_LINK` (a soft link, never followed) |
| `Size` | bytes (0 for a drawer) |
| `Date` | seconds since 1978-01-01 |
| `Prot` | the protection bits (hsparwed) |

`FuzzyRewind(x)` starts again from the first.

**The best matches:** `FuzzySearch(x, pattern, flags, max)` returns the `max` best, best first,
in a `struct FuzzyResults` (`Count` hits in `Hits[]`, and `Matched`: how many matched in all).
Each hit has the entry's fields and its `Score`; the paths are copies, valid until
`FuzzyFreeResults`. Flags: the `FPF_` ones, and:

| Flag | Means |
|---|---|
| `FSF_FILES` | only files |
| `FSF_DRAWERS` | only drawers |
| `FSF_NAME` | match only the name, not the drawers above it |

**Where the index is:** `FuzzyIndexPath(buf, size)` gives the place: the ENV: setting
`FuzzyFind/Index`, else `S:FuzzyFind.index` (not ENVARC:, which is copied to RAM at every boot).

## The index: writing

FuzzyFind's `updatedb` builds the index of the disks. A program can write one of its own (a
catalogue of a CD, of an archive, of the drawers it has visited) the same way:

```c
FuzzyWriter *w = FuzzyNewIndex("RAM:my.index", &err);   /* NULL name: the usual place */
FuzzyAddVolume(w, "Work", volumedate);                   /* all volumes first */
...
e.Path = "Work:Music/mod.foo"; e.PathLen = -1; e.Type = FET_FILE; ...
FuzzyAddEntry(w, &e);                                    /* each file and drawer */
...
err = FuzzyEndIndex(w, TRUE);                            /* TRUE: keep it; FALSE: throw it away */
```

- The index is written to `NAME.new` and renamed over `NAME` by `FuzzyEndIndex(w, TRUE)`, so a
  program reading the old one never sees half a new one.
- **Add a drawer's entries, then walk into its drawers** (as `updatedb` does): neighbours then
  share most of their path, which is what keeps the index small.
- Paths up to 1024 bytes; a longer one is refused (`FuzzyAddEntry` returns FALSE) and the rest
  goes on.

## The index format

Format 1, all numbers big-endian; "v" is a variable-length number: 7 bits a byte, low bits first,
the top bit set on every byte but the last.

```
"FFIX"   UWORD 1 (the format)   UWORD 0 (flags)
ULONG created (seconds since 1978-01-01)   ULONG entries
UWORD volumes, then for each: UBYTE length, the name, ULONG the volume's date
then each entry:
  v shared   bytes the path shares with the path before
  v rest     then that many bytes: the path is the previous one's first "shared" bytes + these
  UBYTE type (FET_)   UBYTE protection   v size   v date
```

Paths are Latin-1, volume names not device names (`Work:` not `DH1:`). A reader refuses a format
it does not know (`FERR_VERSION`). The user's A1200: 21,028 paths in 340 KB (966 KB as a plain
list).

## Speed

**Not measured on a real Amiga yet.** Measured in FS-UAE (2026-10-17) with the index in RAM:, so
the processor's share only, one search of a real A1200's 21,028 paths took:

| Machine (emulated) | One search |
|---|---|
| 68000, 7 MHz, chip memory only (A600) | 14-28 s |
| 68020, 14 MHz, fast memory (A1200) | 3-6 s |

A search reads every entry, so it grows with the size of the index, and on a real machine reading
the index from its drive comes on top (that depends on the drive: not measured). Two things help:

- **A letter fingerprint:** while the entries are read, the library notes which letters each path
  has (32 bits). A path missing one of the pattern's letters is skipped without being scored:
  `goteborg` took half the time of `dpaint`.
- **Narrowing:** a program that keeps the matches (as `fzf` does) only needs to look again at
  them when a letter is added; the first letter is the slow one.

## ARexx

```rexx
CALL ADDLIB('fuzzy.library', 0, -30, 0)
SAY FUZZYSCORE('csrc', 'config/source.c')     /* a number: 0 = no match */
SAY FUZZYLOCATE('dpaint', 5)                  /* the 5 best paths of the index, one a line */
```

| Function | Does |
|---|---|
| `FUZZYSCORE(pattern, text, [options])` | the score: higher is better, 0 no match |
| `FUZZYLOCATE(pattern, [max], [options])` | the best `max` paths of the index (20 if not given), one a line (`'0a'x` between); `''` if none, or if there is no index (`FUZZYERROR()` says) |
| `FUZZYERROR()` | why the last `FUZZYLOCATE` gave nothing |

Options, any of, in one string: `PLAIN CASE ACCENTS TEXT`, and for `FUZZYLOCATE` also
`FILES DRAWERS NAME`. A line of results longer than an ARexx string (65535 bytes) gives `''`;
ask for fewer.

## Versions

While fuzzy.library is young its version is **0** and its **revision** counts up (0.1, 0.2...).
Open version 0 and check the revision before using newer calls:

| Revision | Added |
|---|---|
| 0.1 | everything |

Calls are never removed or moved. The index format has its own number (1), checked by the reader.

## Rules and traps

- **Free what you get:** `FuzzyFreePattern`, `FuzzyCloseIndex`, `FuzzyFreeResults`,
  `FuzzyEndIndex` (always, even to throw an index away).
- **`FuzzyNextEntry`'s `Path` is overwritten by the next call:** copy it to keep it.
  `FuzzySearch`'s hits are copies, safe until `FuzzyFreeResults`.
- **One index handle per task.** Several tasks may open the same index file, each with its own
  `FuzzyOpenIndex`.
- **The index is a snapshot:** files made after `updatedb` ran are not in it. Check a path still
  exists before acting on it.
- **`FuzzySearch` goes through the whole index** each time: in an interactive program keep the
  matches and narrow them as the user types (see FuzzyFind's `fzf`).
