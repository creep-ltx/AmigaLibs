# AmigaLibs

Shared libraries for classic AmigaOS: the `.library` files that programs open from `LIBS:`.
One folder per library, each with its own README, version and change log.

| Library | What it is |
|---|---|
| **[tui.library](tui/README.md)** 0.3 | The toolkit for programs inside the terminal (a CTerm tab, or any Shell window): fast screen updates, colours up to 24-bit, UTF-8, keys, mouse, resizing, pictures. |
| **[json.library](json/README.md)** 0.1 | Read, build and write JSON from C and ARexx: clear errors, paths like `weather.days[0].temp`, Amiga text both ways, no FPU needed. |

Every library comes with a **programmer's guide** (`docs/guide.md`: how to use it, a working first
program, the traps) and a **reference** (`docs/reference.md`: every call in full, with its registers,
laid out like Commodore's Autodocs). Each library's README links to them.

Only libraries live here. Handlers, devices, datatypes and programs each have their own repository.

## Building

Each library builds on its own with bebbo's m68k-amigaos-gcc: `make` inside its folder.
Its README says what comes out and which file to install.

## Used by

- **json.library**: new; no program uses it yet.
- **tui.library**: Sumi, Maki and Markdown (the TextTools repo), Ahiru (the CFile repo), and
  CUtils' `man`, `top` and `ctop`, which build its source into themselves. Those repos expect
  this one beside them (`../AmigaLibs`).

## License

MIT, see [LICENSE](LICENSE).
