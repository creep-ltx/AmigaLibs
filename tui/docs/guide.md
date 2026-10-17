# tui.library programmer's guide

tui.library is for Amiga programs that run **inside a terminal**: a CTerm tab or pane, or a plain
Shell window (CON:). The program gets no window or screen of its own. It draws text into a grid of
cells and reads keys, the mouse and pastes, and the library turns that into the escape sequences the
terminal understands.

This guide explains how to use it. Every call is described in full in the
[reference](reference.md). A complete working program is [tests/tuitest.c](../tests/tuitest.c).

- [What you need](#what-you-need)
- [The first program](#the-first-program)
- [How it works](#how-it-works)
- [Drawing](#drawing)
- [Colours and attributes](#colours-and-attributes)
- [Characters: Latin-1, UTF-8 and plain consoles](#characters-latin-1-utf-8-and-plain-consoles)
- [Events](#events)
- [A program with its own Wait()](#a-program-with-its-own-wait)
- [Running a command and coming back](#running-a-command-and-coming-back)
- [Pictures](#pictures)
- [What CTerm adds](#what-cterm-adds)
- [Versions](#versions)
- [Building it into a program instead](#building-it-into-a-program-instead)
- [Old code: the tui_* names](#old-code-the-tui_-names)
- [Rules and traps](#rules-and-traps)

## What you need

- AmigaOS 2.04 or newer, any 680x0.
- `LIBS:tui.library`: the build for the processor (`build/000`, `020` or `060`, see the README).
- The headers in `include/`: `libraries/tui.h` (types and constants), `proto/tui.h`,
  `inline/tui.h` (the calls, for gcc) and `clib/tui_protos.h` (the prototypes).
  `tui.sfd` describes the calls and their registers, for making headers for other languages
  (Amiga E modules, Blitz, assembler `.i` files: `sfdc` or `fd2pragma` make them).
- Your program's stack: the library's deepest call uses under 0.5 KB.

## The first program

```c
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/tui.h>

struct Library *TuiBase;

int main(void)
{
    struct Tui *t;
    TuiEv e;

    if (!(TuiBase = OpenLibrary(TUINAME, 0))) {
        PutStr("needs tui.library\n");
        return 20;
    }
    if (!(t = TuiOpen(0))) {             /* 0: the program's own Input() */
        PutStr("not in a terminal\n");
        CloseLibrary(TuiBase);
        return 10;
    }
    for (;;) {
        TuiClear(t);
        TuiPut(t, 2, 1, "Hello from the terminal", 23, 15, TC_DEFAULT, TA_BOLD);
        TuiPut(t, 2, 3, "q quits", 7, 8, TC_DEFAULT, 0);
        TuiFlush(t);                     /* now it shows */

        TuiWait(t, &e, -1, 0, NULL);     /* -1: wait for ever */
        if (e.type == TE_END) break;     /* the terminal has gone */
        if (e.type == TE_KEY && (e.key == 'q' || e.key == (TM_CTRL | 'c'))) break;
        if (e.type == TE_RESIZE) TuiResized(t);
    }
    TuiClose(t);                         /* the terminal back as it was */
    CloseLibrary(TuiBase);
    return 0;
}
```

Build with bebbo's gcc: `m68k-amigaos-gcc -Os -noixemul -Wno-pointer-sign -I<tui>/include hello.c -o hello` (the `-Wno-pointer-sign` because the Amiga headers take `UBYTE` strings).

## How it works

**A grid, two copies.** `TuiOpen` makes a grid the size of the terminal. Each cell holds one
character, a foreground colour, a background colour and attributes. The library keeps two copies:
what the program *wants* and what the terminal *shows*. The drawing calls change only the first.
`TuiFlush` compares the two and sends only the cells that differ. A program can redraw its whole
screen after every key and the terminal still gets just the few cells that changed: this is what
keeps it fast over a slow console and over the network.

**Nothing shows until `TuiFlush`.** `TuiWait` flushes by itself before it waits, so a loop that
draws and then waits needs no flush of its own. A program that does a long job between draws must
flush before it starts, or the screen stays as it was.

**Raw mode, its own screen.** `TuiOpen` puts the console in raw mode (keys come one by one, not
echoed), switches off line wrapping, and in CTerm switches to a second screen, so the Shell's
history is untouched when the program ends. A plain console has no second screen: it is cleared
when the program ends. `TuiClose` puts all of it back.

**One `struct Tui` per program.** It belongs to the task that opened it (the library waits for
keys with a message port and signal of that task). Its public fields are read-only:

| Field | |
|---|---|
| `Cols`, `Rows` | the terminal's size now |
| `CTerm` | TRUE in CTerm: colours as given, the mouse, pastes, UTF-8 |
| `UTF8` | TRUE if the terminal takes UTF-8 |
| `CellW`, `CellH` | a cell's size in pixels (CTerm), else 0 |

## Drawing

Columns and rows count from 0, top left. Anything outside the grid is cut off quietly, so a program
need not check the edges itself.

| Call | Draws |
|---|---|
| `TuiPut(t, col, row, s, len, fg, bg, attr)` | `len` bytes of Latin-1 text. `len` must be given (no `-1` here). The fastest way to draw text. |
| `TuiPutUTF8(t, col, row, s, len, fg, bg, attr)` | UTF-8 text; `len < 0` means up to the terminating 0. Returns how many cells it took, so the next piece can follow it. A byte that is not valid UTF-8 is taken as Latin-1. (0.2) |
| `TuiPutC(t, col, row, c, fg, bg, attr)` | one character, Latin-1 or Unicode |
| `TuiFill(t, col, row, w, c, fg, bg, attr)` | `w` copies of one character: lines, bars, empty space |
| `TuiClear(t)` | the whole grid to spaces, colour 7 on the terminal's background |
| `TuiBox(t, which)` | not drawing: returns the character for a box part (`BX_H`, `BX_V`, `BX_TL`, `BX_TR`, `BX_BL`, `BX_BR`, `BX_ARROW`, `BX_DOT`): a line in CTerm, `- \| +` in a plain console. Hand it to `TuiPutC` or `TuiFill`. |
| `TuiCursor(t, col, row, on)` | where the text cursor shows after the next flush, or hidden (`on` = 0). It stays hidden while drawing. |
| `TuiTitle(t, title)` | the tab's title (CTerm only) |
| `TuiBeep(t)` | the terminal's bell |
| `TuiRedraw(t)` | forgets what the terminal shows: the next flush sends every cell. For when something else wrote to the terminal. |

A box, from `tests/tuitest.c`:

```c
TuiPutC(t, x, y, TuiBox(t, BX_TL), 7, TC_DEFAULT, 0);
TuiFill(t, x + 1, y, w - 2, TuiBox(t, BX_H), 7, TC_DEFAULT, 0);
TuiPutC(t, x + w - 1, y, TuiBox(t, BX_TR), 7, TC_DEFAULT, 0);
```

## Colours and attributes

`fg` and `bg` are one of:

| Value | Colour |
|---|---|
| `0` - `15` | the ANSI sixteen (black, red, green, yellow, blue, magenta, cyan, white, then the bright ones). In CTerm, its theme's colours. |
| `TC_DEFAULT` | the terminal's own foreground or background. As a background it lets CTerm's background picture show through. |
| `TC_X(n)` | xterm's 256 colours, `n` 0-255 |
| `TC_RGB(r, g, b)` | any 24-bit colour (0.2) |
| `TC_PEN(n)` | in a plain console, the screen's own pen `n` (0-7) exactly; in CTerm the same as colour `n` (0.3) |

CTerm shows every one as given. A plain console has only its screen's pens (on Workbench: grey,
black, white, blue), so the library picks the nearest one there. Design for CTerm and check the
program once in a plain Shell window: two colours that differ in CTerm can become the same pen.

`attr` is 0 or any of these or'ed together: `TA_BOLD`, `TA_UNDER`, `TA_REV` (reverse),
`TA_ITAL`; and in CTerm only `TA_DIM` and `TA_STRIKE`.

## Characters: Latin-1, UTF-8 and plain consoles

Text is Latin-1 (`TuiPut`) or UTF-8 (`TuiPutUTF8`); a single character for `TuiPutC` or `TuiFill`
is its Unicode number. What reaches the screen depends on the terminal:

- **CTerm** takes UTF-8: every character shows that its font has.
- **A plain console** has Latin-1 only. The library sends the nearest it has: box lines as
  `- | +`, a full block as a reversed space, shades as `:` and `#`, accented letters beyond
  Latin-1 without the accent (`Łódź` as `Lódz`), curly quotes and dashes as `' " -`, arrows as
  `< > ^ v`, `€` as `E`, anything else as `?`.
- **Wide characters** (Chinese, Japanese, Korean, most emoji) take **two cells** (0.5): `TuiPutC`
  and `TuiPutUTF8` fill the cell and the one after it, and the next character goes two cells on.
  In a plain console they show as `?` and a space, so the columns are the same everywhere.
- **Accents written as separate marks** (an e followed by a combining acute, as Macs write file
  names) are put on their letter by `TuiPutUTF8`: é in one cell. A mark on its own in `TuiPutC`
  shows as `?`.

So a program can always write the nicest character and leave the plain console to the library.

**Widths.** `TuiCharWidth(c)` (0.5) says how many cells a character takes in the grid: 2 for wide
ones, 0 for marks, 1 for the rest, -1 for controls. Lay text out with it, not one cell per
character, and wide characters line up:

```c
for (i = 0, x = 0; i < n; i++) {
    TuiPutC(t, x, row, chars[i], 7, TC_DEFAULT, 0);
    x += TuiCharWidth(chars[i]);
}
```

`TuiPutUTF8` returns the cells its text took, so text after it can follow at `col + cells`. The
widths come from **utf8.library** (Unicode 16, `../utf8`) when it is in `LIBS:`, opened the first
time one is needed; without it the library has a table of its own, the same as CTerm's, and a
program built with the toolkit inside (`TUI_STATIC`) always uses that table. CTerm (0.3b53 and up)
asks utf8.library too, so the program and the terminal agree on every character.

**Programs that do not know widths** (that put one character per cell) keep working: when a
character is put over the right half of a wide one, the left half shows `?`, which is what they
always showed.

## Events

`TuiWait(t, &e, us, extra, &got)` waits for the next event:

- `us` is the longest wait in microseconds: `-1` for ever, `0` not at all (only what is there).
- It returns 1 with an event in `e`, or 0 when the time ran out or one of the `extra` signals came
  (`e.type` is then `TE_NONE`).
- `extra` are more signals to wake for (a timer, an ARexx port); `got` (may be NULL) says which of
  them came.
- It flushes the screen before it waits.

| `e.type` | Means | Fields |
|---|---|---|
| `TE_KEY` | a key | `key`: the character (Latin-1 / Unicode) or `K_UP` ... `K_HELP`, `K_F(1)` ... `K_F(12)`, with `TM_SHIFT`, `TM_ALT`, `TM_CTRL` or'ed in. `K_BASE(key)` takes the modifiers off. |
| `TE_MOUSE` | a mouse action (CTerm) | `mact`: `TB_PRESS`, `TB_RELEASE`, `TB_DRAG`, `TB_WHEELUP`, `TB_WHEELDOWN`; `col`, `row`: the cell; `mods`: modifiers held; `secs`, `mics`: when (for double clicks) |
| `TE_PASTE` | pasted text, in one piece (CTerm) | `text`, `len`: valid until the next event; copy it if you keep it |
| `TE_RESIZE` | the terminal changed size | call `TuiResized(t)`, then draw everything again |
| `TE_END` | the terminal has gone (a CTerm tab was closed) | quit. Every later wait returns `TE_END` too. |

Keys in a switch:

```c
switch (e.key) {
case K_UP:                 /* up */
case TM_SHIFT | K_UP:      /* Shift+up */
case TM_CTRL | 's':        /* Ctrl+S */
case K_F(1):               /* F1 */
case 'q':
}
```

**Ctrl-C, D, E and F** are signals on the Amiga, not keys. `TuiWait` turns them into key events
(`TM_CTRL | 'c'` and so on), so a program sees them like any other key and quitting always works,
even in the middle of a wait.

**Resizing.** On `TE_RESIZE`, `Cols` and `Rows` already hold the new size; `TuiResized` makes the
grid that size and clears it (it returns TRUE if the size really changed). Until it is called the
program keeps drawing into the old grid. The size is kept between 10 x 3 and 400 x 200.

The library holds up to 64 events. A program that takes them as they come never fills it.

## A program with its own Wait()

A program that waits for more than the terminal (a window, an ARexx port, a network socket, a
job's messages) keeps its own `Wait()` and adds the library's signal to it (0.3):

```c
ULONG tsig = TuiSignal(t);                /* ask every time: it also sends the key wait out */
TuiFlush(t);
sig = Wait(tsig | arexxsig | SIGBREAKF_CTRL_C);
if (sig & SIGBREAKF_CTRL_C) quit();
if (sig & arexxsig) doarexx();
while (TuiPoll(t, &e))                    /* every event waiting, without waiting */
    handle(&e);
```

- Call `TuiSignal` again before every `Wait()`: it is what sends the next key request to the console.
- Flush before `Wait()` yourself: only `TuiWait` does it for you.
- With `TuiPoll`, Ctrl-C to F stay the program's own signals (the library does not take them).
- The terminal answers within a quarter of a second at most, even with no key pressed.

**A long job** (searching a big file, copying) can ask `TuiKeyWaiting(t)` now and then: TRUE if a
key, click or the end of the terminal is waiting, without taking it. Stop the job, then take the
event with `TuiPoll`.

## Running a command and coming back

To let a command use the terminal (an editor, a compiler, `Execute()`):

```c
TuiSuspend(t);                           /* the terminal as the Shell had it */
SystemTags(cmd, TAG_END);
TuiKeyAfter(t, NULL);                    /* "a key goes back": the output stays to be read */
TuiResume(t);                            /* raw mode again; everything is sent on the next flush */
```

`TuiKeyAfter` is optional, for commands whose output the user should read first. Its message can be
your own (`TuiKeyAfter(t, " Press a key ")`).

## Pictures

Terminals that show pictures (CTerm's sixel, kitty, iTerm images) take them as a stream of bytes.
`TuiRaw(t, col, row, data, len)` sends such bytes straight to the terminal at a cell, after the
program's last flush. The picture now covers cells the library does not know about, so:

1. Draw and flush the text as usual.
2. `TuiRaw` the picture at its top-left cell.
3. When the picture should go, `TuiDirty(t, col, row, w, h)` the cells it covered: the next flush
   sends them again and the picture is gone.

Only send pictures where the terminal takes them (`t->CTerm`, and the program's own check of what
CTerm supports). `CellW` and `CellH` say how many pixels a cell is, to size the picture.

## What CTerm adds

In a plain Shell window a program gets the grid, the Latin-1 characters, the nearest colours,
bold, underline, reverse, italic, keys and resizing. CTerm adds:

- UTF-8, and every colour exactly
- dim and strikethrough
- the mouse (clicks, drags, the wheel) and pastes in one piece
- its own screen for the program, so the Shell's text is untouched
- the tab's title, the cell size in pixels, pictures

A program should work without them: the mouse as an extra, never the only way to do something.

## Versions

While tui.library is young its version is **0** and its **revision** counts up (0.1, 0.2, 0.3).
Open version 0 and check the revision before using newer calls:

| Revision | Added |
|---|---|
| 0.1 | everything not listed below |
| 0.2 | `TuiPutUTF8`, `TC_RGB` |
| 0.3 | `TuiSignal`, `TuiPoll`, `TuiKeyWaiting`, `TC_PEN` |
| 0.4 | `TuiOpenIO` |
| 0.5 | `TuiCharWidth`; wide characters in two cells, accents put on their letters |

```c
TuiBase = OpenLibrary(TUINAME, 0);
if (TuiBase && TuiBase->lib_Version == 0 && TuiBase->lib_Revision < 3) {
    PutStr("needs tui.library 0.3 or newer\n");
    ...
}
```

From 1.0 on the version will count and `OpenLibrary(TUINAME, n)` will do the check. Calls are
never removed or moved, so a program built for an older revision keeps working with every newer one.

## Building it into a program instead

A program that should need nothing in `LIBS:` can have the toolkit inside it: compile with
`-DTUI_STATIC` and link `build/libtuiglue_static.a` and `build/libtui.a` instead of opening the
library (CUtils' `man`, `top` and `ctop` do this). Same calls, same behaviour; the program is about
12 KB bigger and gets fixes only when rebuilt.

## Old code: the tui_* names

Before the library, each program had its own `tui.c` with calls named `tui_open()`, `tui_put()`
and so on, with one terminal per program and no `struct Tui`. `include/tui.h` keeps those names
working over the library, and `build/libtuiglue.a` opens the library the first time it is needed
(and prints "needs tui.library" if it is missing). New programs should use the `Tui*` calls.

## Rules and traps

- **Check `TuiOpen`'s result.** It returns NULL when the program does not run in an interactive
  console: started from Workbench, with output to a file, or from `Run >NIL:`. Fall back to
  plain output there, or say so and quit. `TuiPossible(in)` asks without opening.
- **Input from a pipe.** A program reading data from a pipe (`type file | maki`) has a pipe as
  `Input()`, not the console. Open the console itself (`Open("*", MODE_OLDFILE)`) and pass that
  handle: `TuiOpen(console)`.
- **Output captured** (0.4). A program in backquotes (``Echo `fzf` ``) or with `>file` has no console
  as `Output()`, and inside backquotes `Input()` is `NIL:`. Open the console as `"*"` and give it
  for whichever is not interactive: `TuiOpenIO(in, out)`, 0 meaning `Input()` / `Output()`. Print
  the program's result to `Output()` after `TuiClose`. FuzzyFind's `fzf` does exactly this.
- **Always handle `TE_END`.** When a CTerm tab closes, the console is gone; a program that ignores
  `TE_END` and waits again gets `TE_END` again at once and spins.
- **Always call `TuiClose`**, also on errors, or the Shell is left in raw mode with no cursor.
- **Do not write to `Output()` while the library is open.** It does not know about that text and
  will draw over it. Suspend first, or use `TuiRedraw` after.
- **Nothing before `TuiOpen` and after `TuiClose`.** The `struct Tui` is freed by `TuiClose`.
- **One task.** Use a `struct Tui` only from the task that opened it.
- **Do not call the library from an interrupt** or a task other than the opener: it waits for
  DOS packets.
- **The last cell.** A plain console scrolls when its bottom-right cell is written, so the library
  never writes it there. Do not put anything important in that one cell.
