# tui.library reference

Every call, in the order of the alphabet. The number after each name is the revision it came in
(see [Versions](guide.md#versions)); the registers are for assembler and other languages.
How to use them together: the [programmer's guide](guide.md).

[TuiBeep](#tuibeep) · [TuiBox](#tuibox) · [TuiCharWidth](#tuicharwidth) · [TuiClear](#tuiclear) · [TuiClose](#tuiclose) · [TuiCursor](#tuicursor) · [TuiDirty](#tuidirty) · [TuiFill](#tuifill) · [TuiFlush](#tuiflush) · [TuiKeyAfter](#tuikeyafter) · [TuiKeyWaiting](#tuikeywaiting) · [TuiOpen](#tuiopen) · [TuiOpenIO](#tuiopenio) · [TuiPoll](#tuipoll) · [TuiPossible](#tuipossible) · [TuiPut](#tuiput) · [TuiPutC](#tuiputc) · [TuiPutUTF8](#tuipututf8) · [TuiRaw](#tuiraw) · [TuiRedraw](#tuiredraw) · [TuiResized](#tuiresized) · [TuiResume](#tuiresume) · [TuiSignal](#tuisignal) · [TuiSuspend](#tuisuspend) · [TuiTitle](#tuititle) · [TuiWait](#tuiwait)

## TuiBeep

Ring the terminal's bell (0.1).

```c
TuiBeep(t)
        A0

VOID TuiBeep(struct Tui *);
```

Sends the bell character at once (no flush needed). What it does is the terminal's choice: a flash, a sound, or nothing.

**Inputs**

```
t - from TuiOpen()
```

## TuiBox

The character for a part of a box (0.1).

```c
c = TuiBox(t, which)
D0         A0 D0

ULONG TuiBox(struct Tui *, LONG);
```

Returns the character to draw a box part with: a Unicode line in CTerm, ASCII in a plain console. Draws nothing itself: pass the result to TuiPutC() or TuiFill().

**Inputs**

```
t     - from TuiOpen(); NULL gives the ASCII set
which - BX_H, BX_V (lines), BX_TL, BX_TR, BX_BL, BX_BR (corners),
        BX_ARROW, BX_DOT
```

**Result**

```
c - the character; '?' for an unknown which
```

**Example**

```c
TuiFill(t, 1, 0, w - 2, TuiBox(t, BX_H), 7, TC_DEFAULT, 0);
```

## TuiCharWidth

How many cells a character takes (0.5).

```c
cells = TuiCharWidth(c)
D0                   D0

LONG TuiCharWidth(ULONG);
```

Says how many cells of the grid a character takes, the same answer TuiPutC() and TuiPutUTF8() use: lay text out with it and wide characters line up.

**Inputs**

```
c - a Unicode character (Latin-1 is the same below 256)
```

**Result**

```
cells - 2: wide (Chinese, Japanese, Korean, most emoji)
        1: the rest
        0: a mark that joins the character before (an accent)
       -1: a control (0-31, 127-159) or not a character
```

**Notes:** The widths are utf8.library's (Unicode 16) when it is in LIBS:, else the library's own table, the same as CTerm's. Check lib_Revision >= 5 before calling it; the tui_charwidth() glue name does that and answers 1 for an older library.

## TuiClear

Empty the whole grid (0.1).

```c
TuiClear(t)
         A0

VOID TuiClear(struct Tui *);
```

Fills every cell with a space, colour 7 on TC_DEFAULT, no attributes. Like every drawing call it shows on the next flush, and only the cells that were not empty already are sent.

**Inputs**

```
t - from TuiOpen()
```

**Notes:** Clearing and drawing everything again for each change is the intended way to use the library: TuiFlush() sends only the difference.

## TuiClose

Give the terminal back (0.1).

```c
TuiClose(t)
         A0

VOID TuiClose(struct Tui *);
```

Leaves raw mode, switches the cursor, line wrapping and (in CTerm) the Shell's own screen back on, and frees t. A plain console has no second screen: it is cleared.

**Inputs**

```
t - from TuiOpen(); NULL is safe
```

**Notes:** Call it on every way out of the program, errors included, or the Shell is left in raw mode with no cursor. t may not be used again.

**See also:** [TuiOpen()](#tuiopen), [TuiSuspend()](#tuisuspend)

## TuiCursor

Where the text cursor shows (0.1).

```c
TuiCursor(t, col, row, on)
          A0 D0   D1   D2

VOID TuiCursor(struct Tui *, LONG, LONG, LONG);
```

Sets where the cursor is shown after the next TuiFlush(), or hides it. While the library draws, the cursor is always hidden, so it never jumps about.

**Inputs**

```
t        - from TuiOpen()
col, row - the cell, from 0
on       - TRUE to show it there, FALSE to hide it (the default)
```

## TuiDirty

Cells the terminal may show otherwise now (0.1).

```c
TuiDirty(t, col, row, w, h)
         A0 D0   D1   D2 D3

VOID TuiDirty(struct Tui *, LONG, LONG, LONG, LONG);
```

Marks a rectangle of cells as unknown, so the next TuiFlush() sends them again whatever they hold. Used to take away a picture sent with TuiRaw().

**Inputs**

```
t        - from TuiOpen()
col, row - the top-left cell
w, h     - the size in cells; the part outside the grid is ignored
```

**See also:** [TuiRaw()](#tuiraw), [TuiRedraw()](#tuiredraw)

## TuiFill

A row of one character (0.1).

```c
TuiFill(t, col, row, w, c, fg, bg, attr)
        A0 D0   D1   D2 D3 D4  D5  D6

VOID TuiFill(struct Tui *, LONG, LONG, LONG, ULONG, LONG, LONG, LONG);
```

Puts w copies of c from col along row: lines, bars, empty space.

**Inputs**

```
t        - from TuiOpen()
col, row - the first cell; what falls outside the grid is cut off
w        - how many cells
c        - the character, Latin-1 or Unicode (see TuiPutC())
fg, bg   - colours: 0-15, TC_DEFAULT, TC_X(n), TC_RGB(r,g,b),
           TC_PEN(n)
attr     - TA_BOLD, TA_UNDER, TA_REV, TA_ITAL, TA_DIM, TA_STRIKE
           or'ed together, or 0
```

**Notes:** A full block in a plain console becomes a reversed space: attr's TA_REV is flipped for it.

## TuiFlush

Show what was drawn (0.1).

```c
TuiFlush(t)
         A0

VOID TuiFlush(struct Tui *);
```

Compares the grid the program drew with what the terminal shows and sends only the cells that differ, then places or hides the cursor (TuiCursor()). Does nothing while suspended.

**Inputs**

```
t - from TuiOpen()
```

**Notes:** TuiWait() flushes by itself before it waits. A program with its own Wait() (TuiSignal()) must flush before it. The bottom-right cell of a plain console is never written: the console would scroll.

## TuiKeyAfter

Wait for a key after a command's output (0.1).

```c
TuiKeyAfter(t, msg)
            A0 A1

VOID TuiKeyAfter(struct Tui *, CONST_STRPTR);
```

While suspended: shows msg in reverse under the command's output and waits for one key, so the user can read the output before TuiResume() draws the program again.

**Inputs**

```
t   - from TuiOpen(), suspended
msg - the text, Latin-1; NULL gives " a key goes back "
```

**Notes:** If the terminal goes while waiting, the next TuiWait() returns TE_END.

**See also:** [TuiSuspend()](#tuisuspend), [TuiResume()](#tuiresume)

## TuiKeyWaiting

Is an event waiting? (0.3).

```c
waiting = TuiKeyWaiting(t)
D0                      A0

LONG TuiKeyWaiting(struct Tui *);
```

Says whether a key, click, paste or the end of the terminal is waiting, without taking it or waiting. For a long job that should stop when the user presses a key.

**Inputs**

```
t - from TuiOpen()
```

**Result**

```
waiting - TRUE if something is waiting (take it with TuiPoll()
          or TuiWait()); also TRUE if the terminal has gone
```

**See also:** [TuiPoll()](#tuipoll)

## TuiOpen

Take over the terminal (0.1).

```c
t = TuiOpen(in)
D0          D1

struct Tui *TuiOpen(BPTR);
```

Sets the console to raw mode, switches off wrapping, takes CTerm's second screen, mouse, pastes and UTF-8 where it has them, asks the terminal's size and makes a grid that size, empty.

The public fields of struct Tui (read them, never write them):

```
    Cols, Rows   the size now
    CTerm        TRUE in CTerm
    UTF8         TRUE if the terminal takes UTF-8
    CellW, CellH a cell in pixels (CTerm), else 0
```

**Inputs**

```
in - the console to read keys from; 0 for the program's Input().
     A program whose Input() is a pipe opens "*" itself and passes
     that.
```

**Result**

```
t - the terminal, for every other call; NULL if in or Output() is
    not an interactive console (started from Workbench, output to
    a file, NIL:) or memory ran out
```

**Notes:** t belongs to the calling task: use it only from there.

**See also:** [TuiPossible()](#tuipossible), [TuiClose()](#tuiclose)

## TuiOpenIO

Take over the terminal, keys and screen on given handles (0.4).

```c
t = TuiOpenIO(in, out)
D0            D1  D2

struct Tui *TuiOpenIO(BPTR, BPTR);
```

As TuiOpen(), but the screen is drawn on out instead of Output(). For a program whose output is captured (in backquotes, or redirected to a file) that still draws in the console: open the console as "*" and pass it for in and/or out.

**Inputs**

```
in  - the console to read keys from; 0 for Input()
out - the console to draw on; 0 for Output()
```

**Result**

```
t - the terminal, or NULL if in or out is not an interactive console
```

**Notes:** Check lib_Revision >= 4 before calling it. TuiOpen(in) is TuiOpenIO(in, 0).

**See also:** [TuiOpen](#tuiopen), [TuiClose](#tuiclose)

## TuiPoll

The next event, without waiting (0.3).

```c
got = TuiPoll(t, e)
D0            A0 A1

LONG TuiPoll(struct Tui *, TuiEv *);
```

For a program with its own Wait(): after the TuiSignal() signal came (or at any time), takes the next event if there is one. Call it until it returns 0.

**Inputs**

```
t - from TuiOpen()
e - filled in (see TuiWait() for the event types)
```

**Result**

```
got - 1 with an event in e, 0 with e->type TE_NONE
```

**Notes:** Unlike TuiWait(), Ctrl-C to F are left as the program's own signals; they are not turned into keys.

**See also:** [TuiSignal()](#tuisignal), [TuiWait()](#tuiwait)

## TuiPossible

Could TuiOpen() work here? (0.1).

```c
ok = TuiPossible(in)
D0               D1

LONG TuiPossible(BPTR);
```

Says whether in (0: Input()) and Output() are both an interactive console, without changing anything. For a program that falls back to plain output when it is not in a terminal.

**Inputs**

```
in - as for TuiOpen()
```

**Result**

```
ok - TRUE if TuiOpen() can work
```

## TuiPut

Latin-1 text (0.1).

```c
TuiPut(t, col, row, s, len, fg, bg, attr)
       A0 D0   D1   A1 D2   D3  D4  D5

VOID TuiPut(struct Tui *, LONG, LONG, CONST_STRPTR, LONG, LONG, LONG,
            LONG);
```

Puts len bytes of Latin-1 text from col along row, one cell a byte. The fastest way to draw text.

**Inputs**

```
t        - from TuiOpen()
col, row - the first cell; what falls outside the grid is cut off,
           also when col is negative
s        - the text
len      - its length in bytes: must be given, 0 or more
fg, bg   - colours, see TuiFill()
attr     - attributes, see TuiFill()
```

**Notes:** Control characters show as '?'. For UTF-8 text use TuiPutUTF8().

## TuiPutC

One character (0.1).

```c
TuiPutC(t, col, row, c, fg, bg, attr)
        A0 D0   D1   D2 D3  D4  D5

VOID TuiPutC(struct Tui *, LONG, LONG, ULONG, LONG, LONG, LONG);
```

Puts one character in one cell, or a wide one in two (0.5).

**Inputs**

```
t        - from TuiOpen()
col, row - the cell; outside the grid nothing happens
c        - Latin-1 (0-255) or Unicode (up to 0x10FFFF). A plain
           console gets the nearest it has (box lines as - | +,
           accented letters without the accent, ...). A wide
           character (TuiCharWidth() 2) takes this cell and the next:
           in a plain console '?' and a space; in the last column '?'.
           A combining mark alone shows as '?'.
fg, bg   - colours, see TuiFill()
attr     - attributes, see TuiFill()
```

## TuiPutUTF8

UTF-8 text (0.2).

```c
cells = TuiPutUTF8(t, col, row, s, len, fg, bg, attr)
D0                 A0 D0   D1   A1 D2   D3  D4  D5

LONG TuiPutUTF8(struct Tui *, LONG, LONG, CONST_STRPTR, LONG, LONG,
                LONG, LONG);
```

Puts UTF-8 text from col along row: one cell a character, two for a wide one (0.5), and accents written as separate marks put on their letter (with utf8.library; without it they are left out). A byte that is not valid UTF-8 is taken as Latin-1, so Latin-1 text still shows right.

**Inputs**

```
t        - from TuiOpen(); NULL only counts the cells
col, row - the first cell; what falls outside the grid is cut off
s        - the text
len      - its length in bytes, or -1 for up to its 0
fg, bg   - colours, see TuiFill()
attr     - attributes, see TuiFill()
```

**Result**

```
cells - how many cells the text takes, counting those cut off:
        the next text can follow at col + cells
```

**Notes:** Check lib_Revision >= 2 before calling it.

## TuiRaw

Bytes straight to the terminal (0.1).

```c
TuiRaw(t, col, row, s, n)
       A0 D0   D1   A1 D2

VOID TuiRaw(struct Tui *, LONG, LONG, CONST_STRPTR, LONG);
```

Moves to the cell and writes n bytes as they are, at once: a picture (sixel, kitty, iTerm) or another sequence the library does not know. Afterwards the library forgets where the cursor is and which colours are set.

**Inputs**

```
t        - from TuiOpen()
col, row - where the bytes start
s, n     - the bytes and their count
```

**Notes:** Flush first. The cells under a picture are still the library's: draw nothing there while it shows, and TuiDirty() them to take it away. Send pictures only to terminals that take them.

**See also:** [TuiDirty()](#tuidirty)

## TuiRedraw

Send every cell on the next flush (0.1).

```c
TuiRedraw(t)
          A0

VOID TuiRedraw(struct Tui *);
```

Clears the terminal and forgets what it shows, so the next TuiFlush() sends the whole grid. For when something else wrote to the terminal, or the user asks for a redraw (Ctrl-L).

**Inputs**

```
t - from TuiOpen()
```

## TuiResized

Make the grid the terminal's new size (0.1).

```c
changed = TuiResized(t)
D0                   A0

LONG TuiResized(struct Tui *);
```

After a TE_RESIZE event: makes the grid Cols x Rows, empty, and sends everything on the next flush. The program then draws everything again.

**Inputs**

```
t - from TuiOpen()
```

**Result**

```
changed - TRUE if the size really changed
```

**Notes:** The size is kept between 10 x 3 and 400 x 200.

## TuiResume

Take the terminal again after TuiSuspend() (0.1).

```c
TuiResume(t)
          A0

VOID TuiResume(struct Tui *);
```

Raw mode and the other settings again, asks the size (it may have changed meanwhile) and sends every cell on the next flush.

**Inputs**

```
t - from TuiOpen(), suspended
```

**Notes:** The program should draw everything again: Cols and Rows may differ.

## TuiSignal

The signal to add to the program's own Wait() (0.3).

```c
sigmask = TuiSignal(t)
D0                  A0

ULONG TuiSignal(struct Tui *);
```

For a program that waits for more than the terminal (a window, ARexx, a socket). Sends the request for keys to the console if it is not out already, and returns the signal mask to add to Wait(). When it comes, take the events with TuiPoll().

**Inputs**

```
t - from TuiOpen()
```

**Result**

```
sigmask - the signal, as a mask
```

**Example**

```c
TuiFlush(t);
sig = Wait(TuiSignal(t) | arexxsig | SIGBREAKF_CTRL_C);
...
while (TuiPoll(t, &e)) handle(&e);
```

**Notes:** Call it before every Wait(), not once. The signal comes within a quarter of a second even with no key pressed; then TuiPoll() just returns 0.

**See also:** [TuiPoll()](#tuipoll), [TuiKeyWaiting()](#tuikeywaiting)

## TuiSuspend

Lend the terminal to a command (0.1).

```c
TuiSuspend(t)
           A0

VOID TuiSuspend(struct Tui *);
```

Puts the terminal back as the Shell had it (as TuiClose() does) but keeps t, so a command can run in it. Drawing calls still fill the grid; nothing is sent until TuiResume().

**Inputs**

```
t - from TuiOpen()
```

**See also:** [TuiKeyAfter()](#tuikeyafter), [TuiResume()](#tuiresume)

## TuiTitle

The tab's title (0.1).

```c
TuiTitle(t, title)
         A0 A1

VOID TuiTitle(struct Tui *, CONST_STRPTR);
```

Sets the title of the CTerm tab the program runs in. Does nothing in a plain console. The title is taken away again by TuiClose() and TuiSuspend() and put back by TuiResume() (0.4), so the tab does not go on showing it.

**Inputs**

```
t     - from TuiOpen()
title - the text, 0-terminated
```

## TuiWait

Wait for the next event (0.1).

```c
got = TuiWait(t, e, us, extra, sigs)
D0            A0 A1 D0  D1     A2

LONG TuiWait(struct Tui *, TuiEv *, LONG, ULONG, ULONG *);
```

Flushes the screen, then waits until an event comes, the time runs out or one of the extra signals comes. Ctrl-C to F are taken and given as keys (TM_CTRL | 'c' ...), so quitting always works.

Events (e->type):

```
    TE_KEY     e->key: the character (Latin-1 / Unicode), or K_UP,
               K_DOWN, K_LEFT, K_RIGHT, K_HOME, K_END, K_PGUP,
               K_PGDN, K_INS, K_DEL, K_BS, K_TAB, K_ENTER, K_ESC,
               K_HELP, K_F(1)..K_F(12); with TM_SHIFT, TM_ALT,
               TM_CTRL or'ed in (K_BASE() takes them off)
    TE_MOUSE   (CTerm) e->mact: TB_PRESS, TB_RELEASE, TB_DRAG,
               TB_WHEELUP, TB_WHEELDOWN; e->col, e->row: the cell;
               e->mods: TM_*; e->secs, e->mics: when
    TE_PASTE   (CTerm) e->text, e->len: the pasted text, valid
               until the next event
    TE_RESIZE  Cols and Rows hold the new size: call TuiResized()
               and draw everything again
    TE_END     the terminal has gone: quit. Every later call
               returns TE_END too.
```

**Inputs**

```
t     - from TuiOpen()
e     - filled in
us    - the longest wait in microseconds; -1 for ever, 0 not at
        all
extra - more signals to wake for, or 0
sigs  - set to which of extra came; may be NULL
```

**Result**

```
got - 1 with an event in e; 0 when the time ran out or an extra
      signal came (e->type is TE_NONE)
```

**Notes:** The library keeps up to 64 events waiting.

**See also:** [TuiPoll()](#tuipoll), [TuiSignal()](#tuisignal)

