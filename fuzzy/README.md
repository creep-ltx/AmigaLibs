# fuzzy.library

**Fuzzy matching** as fzf does it, and the **FuzzyFind index**: a catalogue of every file and
drawer on the Amiga's disks. One shared library in `LIBS:`, from C and from **ARexx**.

## What it does

| | |
|---|---|
| **Matching** | `csrc` finds `config/source.c`: the letters in order with gaps, scored as fzf scores them (its v1 way, path scheme); several words in any order |
| **Ranking** | letters at the start of words, letters in a row and the file name count most; `:` is a path separator as on the Amiga. On a real A1200's 21,028 paths the top five equal fzf's |
| **Folding** | case and accents do not matter (`goteborg` finds `Göteborg`) unless asked; Latin-1 |
| **Highlighting** | where each letter matched |
| **The index** | read entry by entry, or searched for the best matches; files, drawers or both; the name only or the whole path |
| **Writing an index** | for FuzzyFind's `updatedb`, or a program's own catalogue; written whole and renamed into place |
| **Small** | about 10 KB; no floating point; a letter fingerprint skips paths that cannot match |
| **ARexx** | `FUZZYSCORE`, `FUZZYLOCATE` |

## For programmers

The [programmer's guide](docs/guide.md) explains matching, the ranking, the index and its format,
with a working first program; the [reference](docs/reference.md) describes every call.

```c
p = FuzzyPrepare("csrc", 0);
score = FuzzyScore(p, "config/source.c", -1, NULL, 0);       /* 0: no match */

x = FuzzyOpenIndex(NULL, &err);                              /* S:FuzzyFind.index */
r = FuzzySearch(x, "dpaint", 0, 5);                          /* the 5 best paths */
```

```rexx
CALL ADDLIB('fuzzy.library', 0, -30, 0)
SAY FUZZYLOCATE('dpaint', 5)
```

The commands built on it, `updatedb`, `locate` and the interactive finder `fzf`, are in the
**FuzzyFind** repository.

## Building

`make` (bebbo's gcc): the library three times and `build/fuzzytest`. `make pctest` runs the core on
the PC under the address and undefined-behaviour sanitizers; `make rank LIST=paths.txt Q="..."`
prints the best matches for queries, to set beside `fzf --filter Q --scheme path`.

| File | For |
|---|---|
| `build/000/fuzzy.library` | 68000, 68010 (every Amiga) |
| `build/020/fuzzy.library` | 68020, 68030, 68040, Emu68 |
| `build/060/fuzzy.library` | 68060 (no 64-bit multiply or divide; runs on a 68040 too) |

AmigaOS 2.04+. No C library inside. **The table order is for ever**: `fuzzy.sfd` and `functable`
in `src/lib.c` in the same order, new calls at the end; the first entry is ARexx's.

## Files

| File | What |
|---|---|
| `fuzzy.sfd` | the calls and their registers |
| `include/libraries/fuzzy.h` | types and constants |
| `src/match.c` | the matching and the ranking |
| `src/index.c` | the index: writing, reading, searching |
| `src/rexx.c`, `src/lib.c`, `src/start.s` | ARexx, the library itself |
| `src/port.h` | memory and files on the Amiga or the PC |
| `docs/guide.md`, `docs/reference.md` | the programmer's guide and the reference |
| `src/hot.s` | the two hottest loops in 68000 assembler (C stands in on the PC) |
| `tests/unit.c`, `rank.c`, `mkindex.c` | the PC tests and tools |
| `tests/prof.c`, `prof2.c` | where a search's time goes, on the Amiga |
| `tests/fuzzytest.c`, `fuzzytest.rexx` | the calls on the Amiga, from C and ARexx; timing |

## State

**0.1b1, 2026-10-17.** Tested:

- **On the PC:** the unit tests (matching, ranking, the index written and read back, a damaged
  index, 100,000 entries) under ASan and UBSan; the ranking set beside fzf 0.74's on a real
  A1200's 21,028 paths (the same top five for `dpaint`, `startup`, `ahiru`, `mod`, `readme`).
- **In FS-UAE:** `fuzzytest` and `fuzzytest.rexx` on the A1200 test machine (68020, OS 3.2.1) and
  the A600 (68000, 2.05); FuzzyFind's commands on both. The guide's first program ran as written.
- **Speed** (21,028 paths, one search; FS-UAE, the index in RAM:, the processor's share only, no
  real drive): 68000 7 MHz chip-only 14-28 s; 68020 14 MHz with fast memory 3-6 s. Not measured on
  a real Amiga.
- **Tuned 2026-10-17:** the two hot loops in 68000 assembler (`src/hot.s`: copying an entry's new
  bytes with their fingerprint; the letter scan), and the name's start kept from entry to entry
  instead of a scan per path. Same results (PC tests; the same match counts on the Amiga). On the
  emulated 68000 **no measurable gain** (within the run-to-run spread of 1-2 s): timing each stage
  (`tests/prof.c`, `prof2.c`) showed the cost spread over many small steps per entry, and looking
  at every letter of every path once is itself about 7 s there. A real gain on a 68000 needs a
  different index layout (proposed, not started).
- **Not yet:** the real A1200.
