---
weight: 26
bookFlatSection: true
title: "Building a MicroEMACS"
---

# Building a MicroEMACS

All versions of MicroEMACS are built from the two sets of
source files. One set is independent of operating system and terminal,
and the other set is dependent.

Compile time options for the independent modules
are selected by setting compilation
switches in `def.h`, and then letting conditional compilation do the
right thing.

The dependent modules are briefly described below.

## Operating System


MicroEMACS runs on several operating systems, including Linux,
FreeBSD, and Windows.  Support code for other operating systems has been lost
(in the distant past, these included CP/M-86 and MS-DOS on the DEC Rainbow,
VMS on the VAX, CP/M-68K, GEMDOS, and FlexOS V60/68K/286).
The following modules contain code dependencies on the operating system:

* `ttyio.c` - low level terminal I/O; independent of terminal type.

* `spawn.c` - subjob creation.

* `fileio.c` - low level file handling.

* `bcopy.s` or `bcopy.asm` - fast byte copy and fill functions.

Adding a new operating system consists mostly of changing these
files, and the header file `sysdef.h`.

## Terminal Support


MicroEMACS supports several kinds of terminals: those
supporting ncurses or termcap, and the native Windows text console
(the code for real-mode PC displays and OS/2 terminal windows has been lost).
The following modules contain code dependencies on the terminal type:

* `tty.c` - high-level terminal support.

* `ttyio.c` - low-level terminal support.

* `ttykbd.c` - keyboard dependencies and extensions.

Changing terminal type consists mostly of changing these files, and the header file `ttydef.h`
These files are located in separate per-terminal subdirectories of the `tty` directory.

Some terminals have memory mapped displays, or interfaces that
act as such.  These include ncurses and Windows text consoles.
Support for these
displays is enabled by setting the MEMMAP switch in `ttydef.h` to 1.
This eliminates the fancy Gosling screen update code in `display.c`,
and enables writing directly to screen memory (or to a screen buffer
that the terminal interface library later writes to the screen).

To
support a new memory-mapped display, you must provide a `putline` function
for writing lines to the display.  On old DOS-base systems, this code
was written in assembly language, but on modern terminals it is
written in C and placed in `tty.c`.

## Building with GCC

To build MicroEMACs on Linux, FreeBSD, or Windows using Cygwin
or MinGW, use these commands:

    mkdir obj
    cd obj
    ../configure
    make # gmake on FreeBSD

You can supply one or more optional parameters to the `configure` command:

`--with-termcap`

:   Use this option to make MicroEMACS use the **terminfo** / **termcap** libraries for
    screen management, instead of the default **ncursesw** library.
    This option will not work on Windows.

`--enable-debug`

:   Use this option to compile and build MicroEMACS with debugging information, so that
    it can be debugged with gdb.

`--with-ruby`

:   Use this option to build support for Ruby extensions into MicroEMACS.
    This option will not work on Windows or FreeBSD.
    See the [**Ruby Extensions**](#ruby-extensions) section above for more information.
