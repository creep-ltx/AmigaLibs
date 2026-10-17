# fuzzy.library reference

Every call, in the order of the alphabet. The number after each name is the revision it came in
(see [Versions](guide.md#versions)); the registers are for assembler and other languages.
How to use them together, the index format and the ARexx functions: the [programmer's guide](guide.md).

[FuzzyAddEntry](#fuzzyaddentry) · [FuzzyAddVolume](#fuzzyaddvolume) · [FuzzyCloseIndex](#fuzzycloseindex) · [FuzzyEndIndex](#fuzzyendindex) · [FuzzyErrorText](#fuzzyerrortext) · [FuzzyFreePattern](#fuzzyfreepattern) · [FuzzyFreeResults](#fuzzyfreeresults) · [FuzzyIndexPath](#fuzzyindexpath) · [FuzzyNewIndex](#fuzzynewindex) · [FuzzyNextEntry](#fuzzynextentry) · [FuzzyOpenIndex](#fuzzyopenindex) · [FuzzyPrepare](#fuzzyprepare) · [FuzzyRewind](#fuzzyrewind) · [FuzzyScore](#fuzzyscore) · [FuzzySearch](#fuzzysearch) · [FuzzyVolume](#fuzzyvolume)

## FuzzyAddEntry

Add a file or drawer to an index being written (0.1).

```c
ok = FuzzyAddEntry(writer, entry)
D0                A0      A1

LONG FuzzyAddEntry(FuzzyWriter *, struct FuzzyEntry *);
```

Writes one entry: Path (PathLen bytes, or -1 for up to its 0), Type, Prot, Size and Date. NameOffset is not used (readers work it out). Add a drawer's entries, then walk into its drawers: neighbours sharing their path keep the index small.

**Inputs**

```
writer - from FuzzyNewIndex()
entry  - the entry
```

**Result**

```
ok - TRUE, or FALSE (the path is over 1024 bytes, or writing failed)
```

**See also:** [FuzzyNewIndex](#fuzzynewindex), [FuzzyEndIndex](#fuzzyendindex)

## FuzzyAddVolume

Name a volume an index covers (0.1).

```c
error = FuzzyAddVolume(writer, name, date)
D0                     A0      A1    D0

LONG FuzzyAddVolume(FuzzyWriter *, CONST_STRPTR, ULONG);
```

Records a volume (its name without the colon, and its date) in the index's header, so a reader can tell which disks it covers. All volumes must come before the first entry.

**Inputs**

```
writer - from FuzzyNewIndex()
name   - "Work" (up to 255 bytes)
date   - the volume's date, seconds since 1978-01-01 (0 if unknown)
```

**Result**

```
error - FERR_OK, or FERR_ORDER (an entry came already),
        FERR_NOMEM (over 64 volumes)
```

**See also:** [FuzzyVolume](#fuzzyvolume)

## FuzzyCloseIndex

Close an index (0.1).

```c
FuzzyCloseIndex(index)
                A0

VOID FuzzyCloseIndex(struct FuzzyIndex *);
```

Closes the file and frees the handle. Hits from FuzzySearch() stay valid (they are copies).

**Inputs**

```
index - from FuzzyOpenIndex(); NULL is safe
```

**See also:** [FuzzyOpenIndex](#fuzzyopenindex)

## FuzzyEndIndex

Finish (or throw away) an index being written (0.1).

```c
error = FuzzyEndIndex(writer, keep)
D0                    A0      D0

LONG FuzzyEndIndex(FuzzyWriter *, LONG);
```

keep TRUE: writes the entry count into the header, closes NAME.new, deletes the old NAME and renames NAME.new to NAME. keep FALSE: closes and deletes NAME.new, the old index stays. The writer is freed either way.

**Inputs**

```
writer - from FuzzyNewIndex()
keep   - TRUE to keep it
```

**Result**

```
error - FERR_OK, or FERR_IO (writing or renaming failed: NAME.new is
        deleted, the old index stays)
```

**Notes:** Always call it, also to give up: it frees the writer.

**See also:** [FuzzyNewIndex](#fuzzynewindex)

## FuzzyErrorText

An error in words (0.1).

```c
text = FuzzyErrorText(error)
D0                    D0

CONST_STRPTR FuzzyErrorText(LONG);
```

Returns an English text for an FERR_ code: "the index is not there", "not a FuzzyFind index"...

**Inputs**

```
error - FERR_OK ... FERR_ORDER
```

**Result**

```
text - read-only; "unknown error" for other numbers
```

## FuzzyFreePattern

Free a prepared pattern (0.1).

```c
FuzzyFreePattern(pattern)
                 A0

VOID FuzzyFreePattern(FuzzyPattern *);
```

Frees what FuzzyPrepare() made.

**Inputs**

```
pattern - from FuzzyPrepare(); NULL is safe
```

**See also:** [FuzzyPrepare](#fuzzyprepare)

## FuzzyFreeResults

Free search results (0.1).

```c
FuzzyFreeResults(results)
                 A0

VOID FuzzyFreeResults(struct FuzzyResults *);
```

Frees the results and every path copy in them.

**Inputs**

```
results - from FuzzySearch(); NULL is safe
```

**See also:** [FuzzySearch](#fuzzysearch)

## FuzzyIndexPath

Where the index is (0.1).

```c
length = FuzzyIndexPath(buf, size)
D0                      A0   D0

LONG FuzzyIndexPath(STRPTR, LONG);
```

Puts the index's place into buf: the ENV: setting FuzzyFind/Index, else S:FuzzyFind.index (FUZZY_INDEX). Every FuzzyFind program and fuzzy.library itself (a NULL name) use this place.

**Inputs**

```
buf  - where to put it
size - its size
```

**Result**

```
length - the length put there (0 if size < 2)
```

**See also:** [FuzzyOpenIndex](#fuzzyopenindex), [FuzzyNewIndex](#fuzzynewindex)

## FuzzyNewIndex

Start writing an index (0.1).

```c
writer = FuzzyNewIndex(name, error)
D0                     A0    A1

FuzzyWriter *FuzzyNewIndex(CONST_STRPTR, LONG *);
```

Opens NAME.new to write a new index into; FuzzyEndIndex() renames it to NAME, so readers never see half an index.

**Inputs**

```
name  - the index's file, or NULL for FuzzyIndexPath()'s place
error - FERR_OK, or why it failed; may be NULL
```

**Result**

```
writer - the writer, or NULL (FERR_NOFILE: NAME.new cannot be made;
         FERR_NOMEM)
```

**See also:** [FuzzyAddVolume](#fuzzyaddvolume), [FuzzyAddEntry](#fuzzyaddentry), [FuzzyEndIndex](#fuzzyendindex)

## FuzzyNextEntry

The next entry of an index (0.1).

```c
ok = FuzzyNextEntry(index, entry)
D0                 A0     A1

LONG FuzzyNextEntry(struct FuzzyIndex *, struct FuzzyEntry *);
```

Reads the next entry into entry: Path, PathLen, NameOffset, Type (FET_FILE, FET_DRAWER, FET_LINK), Prot, Size, Date.

**Inputs**

```
index - from FuzzyOpenIndex()
entry - filled in
```

**Result**

```
ok - TRUE, or FALSE at the end (or where the file is cut short or damaged)
```

**Notes:** Path is the library's and changes with the next call: copy it to keep it.

**See also:** [FuzzyRewind](#fuzzyrewind), [FuzzySearch](#fuzzysearch)

## FuzzyOpenIndex

Open an index (0.1).

```c
index = FuzzyOpenIndex(name, error)
D0                     A0    A1

struct FuzzyIndex *FuzzyOpenIndex(CONST_STRPTR, LONG *);
```

Opens an index and reads its header. The public fields: Count (entries), Created (seconds since 1978-01-01), Volumes.

**Inputs**

```
name  - the index's file, or NULL for FuzzyIndexPath()'s place
error - FERR_OK, or why it failed; may be NULL
```

**Result**

```
index - the index, or NULL: FERR_NOFILE (not there: run updatedb),
        FERR_NOTINDEX, FERR_VERSION (a format this library does not
        know), FERR_NOMEM
```

**Notes:** Each task opens its own.

**See also:** [FuzzyNextEntry](#fuzzynextentry), [FuzzySearch](#fuzzysearch), [FuzzyCloseIndex](#fuzzycloseindex)

## FuzzyPrepare

Prepare a pattern (0.1).

```c
pattern = FuzzyPrepare(text, flags)
D0                     A0    D0

FuzzyPattern *FuzzyPrepare(CONST_STRPTR, ULONG);
```

Makes a pattern from what was typed, to score any number of texts with. Words are split at spaces (up to 16); case and accents are folded away unless the flags say otherwise.

**Inputs**

```
text  - the pattern, Latin-1
flags - 0, or any of:
          FPF_CASE     case matters
          FPF_ACCENTS  accents matter
          FPF_PLAIN    each word as one piece of text, no gaps
          FPF_TEXT     the texts are not paths (no file-name bonus;
                       a word after a space counts more)
```

**Result**

```
pattern - the pattern, or NULL (text NULL, out of memory)
```

**See also:** [FuzzyScore](#fuzzyscore), [FuzzyFreePattern](#fuzzyfreepattern)

## FuzzyRewind

Back to an index's first entry (0.1).

```c
FuzzyRewind(index)
            A0

VOID FuzzyRewind(struct FuzzyIndex *);
```

The next FuzzyNextEntry() gives the first entry again.

**Inputs**

```
index - from FuzzyOpenIndex()
```

**See also:** [FuzzyNextEntry](#fuzzynextentry)

## FuzzyScore

How well a pattern matches a text (0.1).

```c
score = FuzzyScore(pattern, text, len, pos, maxpos)
D0                 A0       A1    D0   A2   D1

LONG FuzzyScore(FuzzyPattern *, CONST_STRPTR, LONG, UWORD *, LONG);
```

Scores the text against the pattern as fzf does (its v1 way, path scheme): every word's letters in order; bonuses for the start of words, letters in a row and the file name; penalties for gaps. All words must match; their scores add up.

**Inputs**

```
pattern - from FuzzyPrepare()
text    - the text, Latin-1
len     - its length, or -1 for up to its 0 (at most 65535 used)
pos     - where to put each matched letter's position, or NULL
maxpos  - how many pos has room for
```

**Result**

```
score - higher is better; 0 is no match; 1 for an empty pattern
```

**Notes:** The positions are in pattern order, word after word: sort them to draw.

**See also:** [FuzzyPrepare](#fuzzyprepare), [FuzzySearch](#fuzzysearch)

## FuzzySearch

The best matches in an index (0.1).

```c
results = FuzzySearch(index, pattern, flags, max)
D0                    A0     A1       D0     D1

struct FuzzyResults *FuzzySearch(struct FuzzyIndex *, CONST_STRPTR, ULONG, LONG);
```

Scores every entry and keeps the max best, best first (ties: the shorter path, then the earlier entry). Results: Count hits in Hits[] (Path, PathLen, NameOffset, Score, Type, Prot, Size, Date; the paths are copies), Matched (how many matched in all). Rewinds the index.

**Inputs**

```
index   - from FuzzyOpenIndex()
pattern - as for FuzzyPrepare()
flags   - FPF_ flags, and FSF_FILES, FSF_DRAWERS (only those),
          FSF_NAME (match only the name)
max     - how many to keep (1 or more)
```

**Result**

```
results - the results, or NULL (out of memory, max < 1)
```

**Notes:** It reads the whole index, so it takes longer the bigger the index and the slower the processor and the drive. Entries missing a letter of the pattern are skipped quickly (a letter fingerprint).

**See also:** [FuzzyFreeResults](#fuzzyfreeresults), [FuzzyScore](#fuzzyscore)

## FuzzyVolume

A volume an index covers (0.1).

```c
name = FuzzyVolume(index, n, date)
D0                 A0     D0 A1

CONST_STRPTR FuzzyVolume(struct FuzzyIndex *, LONG, ULONG *);
```

Returns the n-th volume's name (without the colon) and its date.

**Inputs**

```
index - from FuzzyOpenIndex()
n     - 0 to Volumes - 1
date  - set to the volume's date; may be NULL
```

**Result**

```
name - read-only, or NULL for n out of range
```

**See also:** [FuzzyAddVolume](#fuzzyaddvolume)

## The ARexx entry

The library's first entry (offset -30, before `FuzzyPrepare`) is not a C call: it is the function
ARexx calls for `ADDLIB('fuzzy.library', 0, -30, 0)`, with a RexxMsg in A0. It returns an error
code in D0 and the result string in A0 (and A1). The ARexx functions are in the
[guide](guide.md#arexx).

