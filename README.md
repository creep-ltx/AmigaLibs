# AmigaLibs

Shared libraries for classic AmigaOS: the `.library` files that programs open from `LIBS:`.
One folder per library, each with its own README, version and change log.

| Library | What it is |
|---|---|
| **[tui.library](tui/README.md)** 0.4 | The toolkit for programs inside the terminal (a CTerm tab, or any Shell window): fast screen updates, colours up to 24-bit, UTF-8, keys, mouse, resizing, pictures. |
| **[json.library](json/README.md)** 0.1 | Read, build and write JSON from C and ARexx: clear errors, paths like `weather.days[0].temp`, Amiga text both ways, no FPU needed. |
| **[fuzzy.library](fuzzy/README.md)** 0.1 | Fuzzy matching as fzf does it (`csrc` finds `config/source.c`), and the FuzzyFind index: a catalogue of every file on the disks, searched without touching them. |
| **[utf8.library](utf8/README.md)** 0.1 | UTF-8 text on a Latin-1 Amiga, from C and ARexx: characters and terminal columns counted right, an editor's cursor, search without case or accents, 14 character sets both ways. Unicode 16. |

Every library comes with a **programmer's guide** (`docs/guide.md`: how to use it, a working first
program, the traps) and a **reference** (`docs/reference.md`: every call in full, with its registers,
laid out like Commodore's Autodocs). Each library's README links to them.

Only libraries live here. Handlers, devices, datatypes and programs each have their own repository.

## Building

Each library builds on its own with bebbo's m68k-amigaos-gcc: `make` inside its folder.
Its README says what comes out and which file to install.

## Used by

- **json.library**: new; no program uses it yet.
- **fuzzy.library**: FuzzyFind's `updatedb`, `locate` and `fzf` (the FuzzyFind repo, beside this one).
- **utf8.library**: new; no program uses it yet. tui.library, CTerm, csh and the CUtils are to
  take their UTF-8 work from it.
- **tui.library**: Sumi, Maki and Markdown (the TextTools repo), Ahiru (the CFile repo), and
  CUtils' `man`, `top` and `ctop`, which build its source into themselves. Those repos expect
  this one beside them (`../AmigaLibs`).

## License

MIT, see [LICENSE](LICENSE).
