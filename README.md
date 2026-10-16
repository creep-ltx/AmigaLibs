# AmigaLibs

Shared libraries for classic AmigaOS: the `.library` files that programs open from `LIBS:`.
One folder per library, each with its own README, version and change log.

| Library | What | Version |
|---|---|---|
| [tui/](tui/) | **tui.library**: the toolkit for programs that live inside the terminal (a CTerm tab or pane, or any console window): a screen grid that sends only what changed, 16/256/24-bit colour, UTF-8 with Latin-1 stand-ins, keys, mouse, pastes, resizing, pictures (sixel) | 0.3 |

Only libraries live here. Handlers, devices, datatypes and programs each have their own repository.

## Building

Each library builds on its own with bebbo's m68k-amigaos-gcc: `make` inside its folder.
Its README says what comes out and which file to install.

## Used by

- **tui.library**: Sumi, Maki and Markdown (the TextTools repo), Ahiru (the CFile repo), and
  CUtils' `man`, `top` and `ctop`, which build its source into themselves. Those repos expect
  this one beside them (`../AmigaLibs`).

## License

MIT, see [LICENSE](LICENSE).
