# DEV-SAMPLES-C-PM-CNRMENU

OS/2 PM container control sample demonstrating context menus, multiple views,
container record sharing, source emphasis, direct editing, and sorting.

![CNRMENU Screenshot](/wiki/CNRMENU_001.png)

## Description

`CNRMENU.EXE` builds on the `CNRBASE.EXE` sample by adding:

- **Context menu** — right-click in the container to access View, Sort, and
  Other Window submenus, plus Create New Window and Arrange.
- **Multiple views** — Tree/icon, Name/flowed, Icon, Details, and Text/flowed,
  selectable from the context menu.
- **Container record sharing** — double-clicking a subdirectory opens a new
  window that reuses the same in-memory records, saving memory.
- **Source emphasis** — records visually highlighted while the context menu
  is active.
- **Direct editing** — in-place rename of files via the container MLE editor.
- **Sorting** — sort by name, date/time, or original directory order.

The program starts in Tree/Icon view, showing the current directory (or a
directory given on the command line: `CNRMENU path`). A secondary thread fills
the container so the UI remains responsive during traversal.

## Source structure

```
src/
  CNRMENU.C    - main module: PM init, window class, message loop, wpClient
  CNRMENU.H    - shared header: structures, macros, function prototypes, globals
  CNRMENU.RC   - resources: icon, context menu
  COMMON.C     - SetWindowTitle, Msg, FullyQualify
  CREATE.C     - CreateDirectoryWin, CreateContainer, detail-view column setup
  CTXTMENU.C   - CtxtmenuCreate/Command/SetView/End and helpers
  EDIT.C       - EditBegin/End, RenameFile, RefreshAllContainers
  POPULATE.C   - PopulateContainer thread, ProcessDirectory, shared-record logic
  SORT.C       - SortContainer and three comparison callbacks
  cnrmenu-gcc.def  - GCC module definition (bldlevel, STACKSIZE)
  cnrmenu-ow.lnk  - OpenWatcom wlink script
```

## Requirements

Install the following ArcaOS/OS/2 packages:

```
yum install git gcc make libc-devel binutils watcom-wrc watcom-wlink-hll
```

For OpenWatcom builds, install OpenWatcom 2.0 separately.

## Building with GCC 9.2

Run from the project root on the OS/2 system:

```
compile-gcc.cmd
```

The executable is placed in `bin-gcc\CNRMENU.EXE`.  
The build log is saved to `compile-gcc.log`.

To clean: `compile-gcc.cmd clean`

### GCC notes

- `EMXOMFLD_TYPE`, `EMXOMFLD_LINKER`, and `EMXOMFLD_PRELINK` are set inside
  `compile-gcc.cmd`; they do not need to be set in `CONFIG.SYS`.
- The Watcom Resource Compiler (`wrc`) is used for both resource compilation
  and binding. The IBM `rc.exe` does not support the `-I` flag needed here.
- Header files must come from `libc-devel` (`C:\usr\include`), not from the
  OS/2 Toolkit. Verify with `SET INCLUDE=C:\usr\include` in `CONFIG.SYS`.

## Building with OpenWatcom 2.0

```
compile-ow.cmd
```

The executable is placed in `bin-ow\CNRMENU.EXE`.  
The build log is saved to `compile-ow.log`.

To clean: `compile-ow.cmd clean`

## Version history

| Version | Date       | Notes |
|---------|------------|-------|
| 1.02    | 2026-07-28 | Moved sources to `src/`, added dual GCC/OW build system, fixed GCC 9.2 warnings (`LONGFROMMR`/`SHORT1FROMMR` casts, `(void)pv` idiom, `uintptr_t` pointer cast). |
| 1.02    | 2023-07-27 | Fixed GCC compiler warnings. |
| 1.01    | 2023-05-25 | Adapted to compile on GCC and run on ArcaOS 5.0.7. |
| 1.00    | 1993-01-31 | Original version by Rick Fishman, Code Blazers, Inc. |

## License

BSD 3-Clause License

## Authors

- Martin Iturbide (2023, 2026)
- Rick Fishman, Code Blazers, Inc. (1992–1993)

## Links

- Original source archive: https://archive.org/download/os2_94/os2_94.zip/os2_94%2FI11%2FCNRMNU.ZIP
