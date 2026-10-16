# utf8.library reference

Every call, in the order of the alphabet. The number after each name is the revision it came in
(see [Versions](guide.md#versions)); the registers are for assembler and other languages.
How to use them together, and the ARexx functions: the [programmer's guide](guide.md).

Every call that takes a text takes a pointer and a length in bytes; a length below 0 means "up to
the 0 byte". A byte that is not good UTF-8 is read as the Latin-1 character with that number, so
Amiga text is never refused or lost. A NULL text is an empty one.

The calls that write text (Utf8Case, Utf8FromCharset, Utf8Normalize, Utf8ToCharset) work the same
way: they write at most size - 1 bytes and a 0, never half a character, and return the length the
whole result needs. Call with buf NULL to learn the size, or check `length < size` to know it all
fitted.

[Utf8Case](#utf8case) · [Utf8Charset](#utf8charset) · [Utf8CharsetName](#utf8charsetname) · [Utf8CharWidth](#utf8charwidth) · [Utf8Check](#utf8check) · [Utf8Class](#utf8class) · [Utf8Compare](#utf8compare) · [Utf8Decode](#utf8decode) · [Utf8Encode](#utf8encode) · [Utf8Find](#utf8find) · [Utf8Fit](#utf8fit) · [Utf8FromCharset](#utf8fromcharset) · [Utf8Length](#utf8length) · [Utf8Lower](#utf8lower) · [Utf8Next](#utf8next) · [Utf8NoAccent](#utf8noaccent) · [Utf8Normalize](#utf8normalize) · [Utf8Prev](#utf8prev) · [Utf8ToCharset](#utf8tocharset) · [Utf8Upper](#utf8upper) · [Utf8Valid](#utf8valid) · [Utf8Width](#utf8width)

## Utf8Case

Text in upper or lower case (0.1).

```c
length = Utf8Case(text, len, buf, size, flags)
D0                A0    D0   A1   D1    D2

LONG Utf8Case(CONST_STRPTR, LONG, STRPTR, LONG, ULONG);
```

Writes text into buf in upper case (U8F_UPPER) or lower case (U8F_LOWER), every letter of every script that has case: Å, Σ, Ж, Ǆ. Without either flag it is a plain copy, which still turns Latin-1 bytes into UTF-8.

**Inputs**

```
text  - UTF-8 (a byte that is not UTF-8 is Latin-1)
len   - its length in bytes, or -1: up to its 0
buf   - where to write: UTF-8 and a 0; NULL to measure only
size  - buf's size in bytes
flags - U8F_UPPER or U8F_LOWER
```

**Result**

```
length - the bytes the whole result needs, without the 0
```

**Notes:** One letter becomes one letter: ß stays ß in upper case (Unicode's SS is two letters), and the Turkish dotted and dotless i follow the general rule. A result that does not fit stops at the last whole character that does, with its 0 after it; a result longer than size - 1 (length >= size) was cut.

**See also:** [Utf8Upper](#utf8upper), [Utf8Lower](#utf8lower), [Utf8Compare](#utf8compare)

## Utf8Charset

A character set by its name (0.1).

```c
cs = Utf8Charset(name)
D0               A0

LONG Utf8Charset(CONST_STRPTR);
```

Finds the U8CS_ number of a set from its name, as files, mail headers, web pages and FTP servers give it. Case, '-', '_' and spaces do not matter: "ISO-8859-1", "iso_8859_1" and "Latin1" are the same.

**Inputs**

```
name - the set's name
```

**Result**

```
cs - U8CS_..., or -1 (not a set the library knows)
```

**Notes:** The names known: ISO-8859-1 latin1 l1 amiga iso-ir-100 cp819; ISO-8859-2 latin2 l2 iso-ir-101; ISO-8859-15 latin9 l9 latin0; ISO-8859-5 cyrillic iso-ir-144; ISO-8859-7 greek iso-ir-126 elot928; ISO-8859-9 latin5 l5 turkish iso-ir-148; windows-1250 cp1250; windows-1251 cp1251; windows-1252 cp1252; IBM437 cp437 437 pc8 dos; IBM850 cp850 850; IBM866 cp866 866; KOI8-R koi8; macintosh macroman mac; US-ASCII ascii us ansix3.41968 iso646us; UTF-8 utf8. Since UTF-8 has a number too (U8CS_UTF8), Utf8FromCharset(..., Utf8Charset(name)) works for every name, UTF-8 included.

**See also:** [Utf8CharsetName](#utf8charsetname), [Utf8ToCharset](#utf8tocharset), [Utf8FromCharset](#utf8fromcharset)

## Utf8CharsetName

A character set's name (0.1).

```c
name = Utf8CharsetName(cs)
D0                     D0

CONST_STRPTR Utf8CharsetName(LONG);
```

Returns the usual name of set cs, as it is written in files and mail ("ISO-8859-2", "windows-1252", "IBM437").

**Inputs**

```
cs - U8CS_...
```

**Result**

```
name - the name (the library's own: do not change it), or NULL past
       the last set
```

**Notes:** To list every set (for a menu): for (cs = 0; (name = Utf8CharsetName(cs)); cs++) ...

**See also:** [Utf8Charset](#utf8charset)

## Utf8CharWidth

How many columns a character takes (0.1).

```c
columns = Utf8CharWidth(cp)
D0                      D0

LONG Utf8CharWidth(ULONG);
```

Returns how many columns of a terminal (or of any fixed-width text) the character takes, from Unicode 16's East Asian Width: Chinese, Japanese and Korean characters and most emoji take two, accents and other marks that join the character before take none.

**Inputs**

```
cp - a code point
```

**Result**

```
columns - 2 wide, 1, 0 (joins the one before), or -1: a control
          (0-31, 127-159), a surrogate or not a code point
```

**Notes:** "Ambiguous" characters (Greek and Cyrillic in East Asian fonts, some symbols) are 1, as in western terminals. For the width of a whole text use Utf8Width(), which also knows that a flag's two halves and an emoji sequence are one character.

**See also:** [Utf8Width](#utf8width), [Utf8Fit](#utf8fit), [Utf8Class](#utf8class)

## Utf8Check

What kind of text it is (0.1).

```c
kind = Utf8Check(text, len)
D0              A0    D0

LONG Utf8Check(CONST_STRPTR, LONG);
```

Tells plain ASCII, UTF-8 and 8-bit text (Latin-1 or another set) apart, for a program that has to guess what a file holds.

**Inputs**

```
text - the bytes
len  - their length, or -1: up to the 0
```

**Result**

```
kind - U8_ASCII:  only bytes below 128 (the same in every set)
       U8_UTF8:   good UTF-8 with characters beyond ASCII
       U8_LATIN1: has bytes that are not UTF-8, so 8-bit text
```

**Notes:** 8-bit text that happens to be good UTF-8 is rare in practice: two or more accented letters in a row that form a UTF-8 sequence. When reading a file in pieces, a sequence cut at the end of a piece is "not UTF-8": Utf8Valid() tells where it starts, so the piece can end there.

**See also:** [Utf8Valid](#utf8valid), [Utf8FromCharset](#utf8fromcharset)

## Utf8Class

What a character is (0.1).

```c
bits = Utf8Class(cp)
D0               D0

ULONG Utf8Class(ULONG);
```

Tells what kind of character cp is: for word-wise cursor moves and double-click selection (UC_WORD), checking input, or drawing. One of the kind bits is set, and the others where they apply.

**Inputs**

```
cp - a code point
```

**Result**

```
bits - one of
         UC_LETTER   a letter of any script (also Chinese characters)
         UC_DIGIT    0-9 in any script (Arabic, Devanagari, fullwidth...)
         UC_NUMBER   other numbers: ½ ² Ⅻ
         UC_PUNCT    punctuation and symbols, emoji too
         UC_SPACE    spaces (also tab and line ends, with UC_CONTROL)
         UC_MARK     accents and other marks that join the one before
         UC_CONTROL  controls, formats, private use, unassigned
       and
         UC_UPPER    a letter with a lower case form
         UC_LOWER    a letter with an upper case form
         UC_WIDE     two columns
       0 for a surrogate or past 0x10FFFF
```

**Notes:** UC_WORD is UC_LETTER | UC_DIGIT | UC_NUMBER | UC_MARK: the characters that make up a word. Above U+FFFF the kind is a coarse one (emoji are UC_PUNCT, the historic scripts UC_LETTER, their marks UC_MARK).

**See also:** [Utf8CharWidth](#utf8charwidth), [Utf8Upper](#utf8upper)

## Utf8Compare

Compare two texts (0.1).

```c
order = Utf8Compare(a, alen, b, blen, flags)
D0                  A0 D0    A1 D1    D2

LONG Utf8Compare(CONST_STRPTR, LONG, CONST_STRPTR, LONG, ULONG);
```

Compares a and b character by character, by their Unicode numbers. An accented letter written as one character (å) and as a letter and an accent (a + ring above, as Macs write file names) are the same, and so is a Latin-1 byte and the same letter in UTF-8.

**Inputs**

```
a, b       - the texts (UTF-8; bytes that are not UTF-8 are Latin-1)
alen, blen - their lengths, or -1: up to the 0
flags      - 0, or:
             U8F_NOCASE:   upper and lower case the same (A = a,
                           Σ = σ = ς)
             U8F_NOACCENT: letters with and without accents the
                           same (é = e, Å = A, Ł = L)
```

**Result**

```
order - -1: a comes first, 0: the same, 1: b comes first
```

**Notes:** The order is Unicode's, not a language's alphabet: Swedish Å Ä Ö come after Z only because their numbers are higher, and German Ä comes after Z too (where a German dictionary puts it with A). Enough for sorting file names; not a dictionary sort.

**See also:** [Utf8Find](#utf8find), [Utf8Normalize](#utf8normalize)

## Utf8Decode

Read one character (0.1).

```c
used = Utf8Decode(text, len, cp)
D0              A0    D0   A1

LONG Utf8Decode(CONST_STRPTR, LONG, ULONG *);
```

Reads the character at text and gives its code point. The building block for a program's own loops over UTF-8.

**Inputs**

```
text - where the character starts
len  - the bytes there are, or -1: up to a 0 byte
cp   - where to put the code point (may be NULL)
```

**Result**

```
used - 1 to 4: the character's bytes, good UTF-8
       -1: the byte is not UTF-8 (a stray byte, a sequence cut
           short, too long a way to write a small one, a
           surrogate); *cp is that byte as Latin-1, and the next
           character is one byte on
        0: nothing left (len 0, or a 0 byte with len -1)
```

**Notes:** A loop over every code point:

```c
while ((k = Utf8Decode(p, end - p, &c)) != 0) {
    ...                     /* c: the character */
    p += k < 0 ? 1 : k;
}
```

This gives code points. What a reader sees as one character can be several (a letter and an accent, a flag): Utf8Next() steps by those.

**See also:** [Utf8Encode](#utf8encode), [Utf8Next](#utf8next), [Utf8Valid](#utf8valid)

## Utf8Encode

Write one character (0.1).

```c
bytes = Utf8Encode(cp, buf)
D0               D0  A0

LONG Utf8Encode(ULONG, STRPTR);
```

Writes code point cp as UTF-8.

**Inputs**

```
cp  - a code point, 0 to 0x10FFFF
buf - room for 4 bytes (no 0 is written after them)
```

**Result**

```
bytes - 1 to 4, or 0: cp is a surrogate (0xD800-0xDFFF) or past
        0x10FFFF, and nothing was written
```

**See also:** [Utf8Decode](#utf8decode)

## Utf8Find

Find a text in a text (0.1).

```c
pos = Utf8Find(text, len, find, flen, flags, matchlen)
D0             A0    D0   A1    D1    D2     A2

LONG Utf8Find(CONST_STRPTR, LONG, CONST_STRPTR, LONG, ULONG, LONG *);
```

Looks for find in text, as Utf8Compare() compares: accents written either way are the same, and with the flags case and accents can be left out. "gote" finds "Göteborg" with U8F_NOCASE | U8F_NOACCENT.

**Inputs**

```
text     - where to look (UTF-8; bytes that are not UTF-8 are Latin-1)
len      - its length, or -1: up to the 0
find     - what to look for
flen     - its length, or -1
flags    - 0, U8F_NOCASE, U8F_NOACCENT, or both
matchlen - where to put the length in bytes of what was found (may
           be NULL)
```

**Result**

```
pos - the byte where the first match starts, or -1 (not found);
      0 with *matchlen 0 when find is empty
```

**Notes:** A match starts and ends on whole characters: "e" does not find the e of an é written as e + accent unless U8F_NOACCENT is given. The match can be longer or shorter in bytes than find ("e" finds "é", two bytes, with U8F_NOACCENT): use *matchlen, not flen, to mark or replace it. For the next match search again from pos + *matchlen.

**See also:** [Utf8Compare](#utf8compare)

## Utf8Fit

How much fits in some columns (0.1).

```c
bytes = Utf8Fit(text, len, cols, width)
D0            A0    D0   D1    A1

LONG Utf8Fit(CONST_STRPTR, LONG, LONG, LONG *);
```

Finds how much of the start of text fits in cols columns: for cutting a line at the edge of a window, or finding which character a mouse click at a column lands on.

**Inputs**

```
text  - UTF-8
len   - its length, or -1: up to the 0
cols  - the columns there are
width - where to put the columns the part that fits takes (may be
        NULL); it can be one less than cols when a wide character
        did not fit
```

**Result**

```
bytes - the length of the part that fits: never half a character,
        and a character's accents always with it
```

**Notes:** Controls count 0 columns, as in Utf8Width(). To cut a line with "..." at the end: Utf8Fit(text, len, cols - 3, &w), write those bytes and the three dots.

**See also:** [Utf8Width](#utf8width), [Utf8CharWidth](#utf8charwidth)

## Utf8FromCharset

8-bit text to UTF-8 (0.1).

```c
length = Utf8FromCharset(text, len, buf, size, cs)
D0                       A0    D0   A1   D1    D2

LONG Utf8FromCharset(CONST_STRPTR, LONG, STRPTR, LONG, LONG);
```

Turns text in an 8-bit set into UTF-8: Amiga text (U8CS_LATIN1) for a web page or a mail, a PC's ANSI art (U8CS_CP437), a Polish file (U8CS_LATIN2).

**Inputs**

```
text - the 8-bit text
len  - its length, or -1: up to the 0
buf  - where to write: UTF-8 and a 0; NULL to measure only
size - buf's size in bytes
cs   - the set the text is in: U8CS_...
```

**Result**

```
length - the bytes the whole result needs, without the 0; -1 when cs
         is not a set
```

**Notes:** A byte the set has nothing for (0x81 in windows-1252, any byte from 128 up in U8CS_ASCII) becomes U+FFFD, the replacement character. With U8CS_UTF8 the text is copied, and bytes that are not UTF-8 become their Latin-1 letters: good UTF-8 out whatever came in. A result that does not fit stops at the last whole character, with its 0.

**See also:** [Utf8ToCharset](#utf8tocharset), [Utf8Charset](#utf8charset), [Utf8Check](#utf8check)

## Utf8Length

How many characters (0.1).

```c
count = Utf8Length(text, len)
D0                A0    D0

LONG Utf8Length(CONST_STRPTR, LONG);
```

Counts the characters as a reader sees them: a letter with its accents is one (also when written as a letter and separate accents), a flag (two regional letters) is one, an emoji family joined by zero-width joiners is one, and CR LF is one.

**Inputs**

```
text - UTF-8 (a byte that is not UTF-8 is one Latin-1 character)
len  - its length in bytes, or -1: up to its 0
```

**Result**

```
count - the characters
```

**Notes:** To count code points instead, loop with Utf8Decode(). The rules are a short form of Unicode's grapheme clusters (UAX #29): marks, joiners, variation selectors, skin tones, tags and Hangul vowels join the character before; Indic conjuncts are not put together.

**See also:** [Utf8Next](#utf8next), [Utf8Width](#utf8width)

## Utf8Lower

A character in lower case (0.1).

```c
lower = Utf8Lower(cp)
D0                D0

ULONG Utf8Lower(ULONG);
```

Returns the lower case form of a character: Å gives å, Σ gives σ, Ж gives ж.

**Inputs**

```
cp - a code point
```

**Result**

```
lower - its lower case form, or cp itself (no case, or already lower)
```

**See also:** [Utf8Upper](#utf8upper), [Utf8Case](#utf8case), [Utf8Class](#utf8class)

## Utf8Next

To the next character (0.1).

```c
next = Utf8Next(text, len, pos)
D0              A0    D0   D1

LONG Utf8Next(CONST_STRPTR, LONG, LONG);
```

Moves from the character at byte pos to the start of the next one, as a reader counts them (see Utf8Length): what an editor's cursor-right does.

**Inputs**

```
text - UTF-8
len  - its length, or -1: up to the 0
pos  - where a character starts
```

**Result**

```
next - where the next starts; len at the end (and for pos >= len)
```

**Notes:** pos should be where a character starts (0, or what Utf8Next or Utf8Prev gave). From the middle of one it moves a byte at a time.

**See also:** [Utf8Prev](#utf8prev), [Utf8Length](#utf8length)

## Utf8NoAccent

A letter without its accents (0.1).

```c
plain = Utf8NoAccent(cp)
D0                   D0

ULONG Utf8NoAccent(ULONG);
```

Returns the letter without accents: é gives e, Å gives A, ệ (two accents) gives e, Й gives И, が gives か. Letters with a stroke that Unicode does not take apart are covered too: Ø, Ł, Đ, Ħ, Ŧ, ı and a few more give O, L, D, H, T, i.

**Inputs**

```
cp - a code point
```

**Result**

```
plain - the letter without accents, or cp itself
```

**Notes:** Covers Latin, Greek, Cyrillic, Japanese kana and the signs Ω, K, Å that are other letters in disguise. Letters that are not a letter with an accent stay as they are: Æ, ß, Þ (Utf8ToCharset writes AE, ss for the first two when a set lacks them).

**See also:** [Utf8Find](#utf8find), [Utf8Compare](#utf8compare), [Utf8ToCharset](#utf8tocharset)

## Utf8Normalize

Accents together or apart (0.1).

```c
length = Utf8Normalize(text, len, buf, size, flags)
D0                     A0    D0   A1   D1    D2

LONG Utf8Normalize(CONST_STRPTR, LONG, STRPTR, LONG, ULONG);
```

Writes text with every accent put on its letter (U8F_COMPOSE: å as one character, as Amigas, Windows and the web write it) or taken off as a separate mark (U8F_DECOMPOSE: a + ring above, as Macs may write file names). Use it on names from a Mac (over NFS or SMB) before showing or comparing them.

**Inputs**

```
text  - UTF-8 (a byte that is not UTF-8 is Latin-1)
len   - its length, or -1: up to its 0
buf   - where to write: UTF-8 and a 0; NULL to measure only
size  - buf's size in bytes
flags - U8F_COMPOSE or U8F_DECOMPOSE (0: a plain copy that turns
        Latin-1 bytes into UTF-8)
```

**Result**

```
length - the bytes the whole result needs, without the 0
```

**Notes:** For Latin, Greek, Cyrillic, kana and Hangul it gives what Unicode's NFC and NFD give (checked against Python's for every character there). Other scripts pass through unchanged, and marks are not put in Unicode's order when several are on one letter in an unusual order.

**See also:** [Utf8Compare](#utf8compare), [Utf8ToCharset](#utf8tocharset)

## Utf8Prev

To the character before (0.1).

```c
prev = Utf8Prev(text, len, pos)
D0              A0    D0   D1

LONG Utf8Prev(CONST_STRPTR, LONG, LONG);
```

Moves from byte pos to the start of the character before it: what an editor's cursor-left and backspace need. Accents, flags and emoji sequences are passed over whole.

**Inputs**

```
text - UTF-8
len  - its length, or -1: up to the 0
pos  - a position in the text
```

**Result**

```
prev - where the character before pos starts; 0 at the start. From
       the middle of a character: where that one starts
```

**See also:** [Utf8Next](#utf8next)

## Utf8ToCharset

UTF-8 to an 8-bit set (0.1).

```c
length = Utf8ToCharset(text, len, buf, size, cs, flags)
D0                     A0    D0   A1   D1    D2  D3

LONG Utf8ToCharset(CONST_STRPTR, LONG, STRPTR, LONG, LONG, ULONG);
```

Turns UTF-8 into an 8-bit set: Latin-1 for an Amiga screen, a console window or a file other Amiga programs read; Latin-2 for a Polish Amiga; CP437 for a PC BBS. Accents written as separate marks are put on their letters first, so a Mac's "a + ring" still becomes Latin-1's å.

**Inputs**

```
text  - UTF-8 (a byte that is not UTF-8 is Latin-1)
len   - its length, or -1: up to its 0
buf   - where to write: the set's bytes and a 0; NULL to measure
size  - buf's size in bytes
cs    - the set: U8CS_...
flags - 0: the nearest for what the set lacks; U8F_STRICT: '?'
```

**Result**

```
length - the bytes the whole result needs, without the 0; -1 when cs
         is not a set
```

**Notes:** Without U8F_STRICT, what the set lacks becomes the nearest it has: the letter without its accent (Ł gives L in Latin-1), typographic quotes and dashes give " ' -, … gives ..., € gives EUR (in sets without €), ™ gives TM, ≤ gives <=, box lines give - | +, a full block #, a fullwidth Ａ an A. What has nothing near (Chinese, emoji) gives '?'. Invisible characters (zero-width joiners, variation selectors, a byte order mark) give nothing. One character can give up to three bytes, so the result can be longer than the text. A result that does not fit stops at a whole character, with its 0.

**See also:** [Utf8FromCharset](#utf8fromcharset), [Utf8Charset](#utf8charset), [Utf8NoAccent](#utf8noaccent)

## Utf8Upper

A character in upper case (0.1).

```c
upper = Utf8Upper(cp)
D0                D0

ULONG Utf8Upper(ULONG);
```

Returns the upper case form of a character: å gives Å, σ and ς give Σ, ÿ gives Ÿ.

**Inputs**

```
cp - a code point
```

**Result**

```
upper - its upper case form, or cp itself (no case, already upper, or
        no single-letter upper case: ß)
```

**See also:** [Utf8Lower](#utf8lower), [Utf8Case](#utf8case), [Utf8Class](#utf8class)

## Utf8Valid

Where the text stops being UTF-8 (0.1).

```c
pos = Utf8Valid(text, len)
D0             A0    D0

LONG Utf8Valid(CONST_STRPTR, LONG);
```

Checks that text is good UTF-8 (RFC 3629: no stray bytes, nothing cut short, no needlessly long forms, no surrogates, nothing past U+10FFFF).

**Inputs**

```
text - the bytes
len  - their length, or -1: up to the 0
```

**Result**

```
pos - -1: all good; else the byte where the first fault starts
```

**See also:** [Utf8Check](#utf8check), [Utf8Decode](#utf8decode)

## Utf8Width

How many columns a text takes (0.1).

```c
columns = Utf8Width(text, len)
D0                 A0    D0

LONG Utf8Width(CONST_STRPTR, LONG);
```

Returns how many columns of a terminal the text takes: 日本 takes 4, é written as e + accent takes 1, a flag takes 2. For laying out columns, centring a title, or knowing whether a line fits.

**Inputs**

```
text - UTF-8 (a byte that is not UTF-8 is a Latin-1 character, 1 column)
len  - its length, or -1: up to the 0
```

**Result**

```
columns - the columns
```

**Notes:** Controls (tab, line ends, escape) count 0: where a tab goes is the program's business. A character takes its first code point's width; a flag takes 2. Emoji with a variation selector (U+FE0F) after a normally narrow symbol are counted 1, as most terminals do.

**See also:** [Utf8Fit](#utf8fit), [Utf8CharWidth](#utf8charwidth)
