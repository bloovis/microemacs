---
weight: 2
bookFlatSection: true
title: "Basic Concepts"
---

# Some Basic Concepts

This section describes a number of basic concepts,
which will be used again and again in the following descriptions.
Most of it should be familiar to EMACS users.
New users are advised not to get stuck reading this section,
because most of the information will become clear the first time you
play with the editor on a computer.

## Screen Layout


MicroEMACS divides the screen into two areas. The very last
line on the screen, which is used to display messages and
ask one line questions, is called the *echo line*.
Typing in the echo line
is like typing commands to the operating system.  More details later
in the section [**The Echo Line**](#the-echo-line).

The remainder of the screen is filled with text *windows*.
When MicroEMACS is first started, there will be one or two text windows,
depending on how many files you specify on the invocation line.
If there is only one window, it fills the screen.  If there are two
windows, each occupies half the screen.
Windows can be created, deleted, and adjusted in
size using MicroEMACS commands.

## Mode Lines

The last line of a window (the line, usually in reverse video,
that starts with `MicroEMACS`) is
the *mode line*  for the window.
It contains information about what is being displayed in the
window. Mode lines separate windows from each other and from the
echo line.

Three important bits of information are displayed in the mode line.
These are the buffer *mode*  (if any) in parentheses,
the *buffer name* ,
the *file name* ,
and the *buffer changed flag*.

A window is always displaying a buffer.
The name of the buffer being
displayed in the window is shown in the mode line, right after the
`MicroEMACS` and the buffer mode.

A buffer may have a file associated with it.
Usually this is the last file read into or written from the
buffer. If a file is associated with the buffer, the name of the
file is displayed in the mode line, prefixed by `File:`.
On operating systems in which file names are case insensitive
(for example, VMS) the file name will always be in lower case.

MicroEMACS knows if a buffer has been changed since it was last
written to its associated file. It informs you of this fact by displaying
a `*` in the mode line immediately to the left of the `MicroEMACS`
string.

## Keyboard Conventions


Internally, MicroEMACS represents keyboard characters in a special
11 bit code. This code consists of 8 bits of character data (ASCII plus
whatever characters your terminal puts in the right side graphics set), and
3 flags, the *CONTROL* flag,
the *META* flag,
and the *CTLX* flag.
Combinations of the three flags are possible, with the exception that
META and CTLX cannot be set at the same time.

This manual represents 11 bit characters in a standard way. If you
have a character `A`, then that character with
the *CONTROL* bit set is
written as **C-A**. That character with the
*META* bit set is written as **M-A**.
That character with the *CTLX* bit set is written as **C-X A**.
If both the META and CONTROL bits are set, it is written as **M-C-A**.
If both the CTLX and CONTROL bits are set, it is written as **C-X C-A**.

Terminals (usually) cannot deal directly deal with the 11 bit
character set.
Characters with the flag bits set are entered by prefixing the
desired character with another character, as follows:

* To get a character with the *CONTROL* bit set,
hold down the `CTRL` key while entering the character.
An alternative method
is to prefix the character with `Control-^` (the
word `Control` here, and in the next few examples, means the key marked
`Control` or `CTRL` on the ASCII keyboard).

* To get a character with the
*META* bit set, prefix the character
with `Control-[`, `Escape`, or `ESC`.  An alternative method that
works on FlexOS is to
hold down the `ALT` key while entering the character.  This method
also works on PC-DOS, but only with `ALT-A` through `ALT-Z`.

* To get a character with the
*CTLX* bit set, prefix the character
with `Control-X`.

* To get a character with the *CONTROL*
and *META* bits set, prefix the character
with `Control-\`.  An alternative method is to prefix the character
with `ESC`, then hold down `CTRL` while entering the character.

* Those of you who understand ASCII are probably asking what you get
if you prefix a character with `Control-]`.
This character is not a prefix;
it is reserved for future use as an "exit from recursive editing level"
command.

CONTROL characters
are very important, so there is a shorthand way
of typing them, as follows:

* Characters between `Control-@` (`Control-Space` on some keyboards)
and `Control-Z` are mapped
to the characters `@` through `Z` with the *CONTROL* bit set.

* Characters between
`Control-a` and `Control-z` are mapped to
the characters `A` through `Z`
with the *CONTROL* bit set.

This shorthand means that that **M-C-V**, for example, can be entered 
either as
`ESC` followed by `Control-V`, or as `Control-\` followed by `V`.
As another example, **C-X C-B** is entered as `Control-X` followed
by `Control-B`.

### The PC Keyboard


MicroEMACS understands all of the cursor keypad and function keys
on PC compatible machines, and the equivalent keys on
other machines that run FlexOS (such as
the VME/10).  These keys include the function keys
(F1-F10), shifted function keys (S-F1 to S-F10),
cursor keys (Up, Down, Left,
Right, PgUp, PgDn, Home, End), Delete, and Insert.
These keys may be combined with the ALT and CTRL keys.

On PCs, you can use the `ALT` key
 as the
META flag, instead of the `ESC` key.
For example, the **M-L** command
(lower-word) can be executed in either of two ways:

* Hit `ESC` followed by `L`.

* Hold down `ALT` and hit `L`.

On PC-DOS and Linux, the `ALT` key only works with alphabetic
characters (`A` through `Z`).
Using `ALT` and `CTRL` together is also not supported.

On FlexOS, the `ALT` key works with both alpha and non-alpha
characters, and simultaneously with the `CTRL` key.
Thus, the command **M-C-V** (display-version) can be executed by
holding down `ALT` and `CTRL`, while pressing
`V` (not an easy task, in this writer's opinion).

On the VME/10, some of the cursor keys are not available, due to
the inherent physical limitations of the keyboard itself.

See the [**Wall Chart**](#wall-chart) section of this manual for the
predefined bindings of some the special PC keys.

### The Zenith Z-19/29 Keyboard



MicroEMACS treats the numeric keypad on the Z-19/29 similarly to the
EDT/VMS customizations in common use at DRI.
It does this by putting the terminal in "alternate keypad mode", so that
the keypad produces escape sequences instead of numbers.
The arrow keys (2, 4, 6 and 8) move one line
or one character at a time; when shifted they move by pages or word.
When preceded by the F6 key (BLUE on Z-19), the arrow keys
move to the beginning
or end of the current line or buffer.
The 5 key is the same as **C-S** (forw-search), and shift-5 is
search-again.  See the [**Wall Chart**](#wall-chart) section of this manual
for further key bindings for the Z-19/29.

Not all of the key bindings that allow the shifted arrows to work like EDT
 are
built into MicroEMACS.  This is because they conflict with the traditional
definitions of **M-B** and **M-C**.  Therefore, these rebindings
are placed in a profile.
If you want MicroEMACS
to read these bindings when it starts up, and you don't care about
losing the old meanings of **M-B** and **M-C**, then
copy **edt.pro** to the default profile name, as described below
in the [**Profiles**](#profiles) section.

### The VT-100 Keyboard

MicroEMACS understands the arrow keys and the keypad on VT-100
style keyboards. Only the PF1 through PF4 keys on the keypad have any
special meaning; the digits,
the `-` key, and the `,` key are treated just
like the non keypad versions of those characters.
The `Enter` key is treated
exactly like the `Return` key.

The arrow keys may be used for moving around. The left arrow is the
same as  **C-B**,
the right arrow is the same as **C-F**, the up arrow is the same
as **C-P**, and the down arrow is the same as **C-N**.

The four function keys are command keys.
The `PF1` key is the same as **M-X**
(the `PF1` key is where the `Gold` key normally
resides on DEC products, so
it is a good choice for the key that asks for extended commands).
The `PF2` key is the same as **C-Q** (quote character).
The `PF3` key is the same as **C-S** (search forward).
The `PF4` key is the same as **C-R** (search reverse). 
These assignments have not been proven optimal, so they may get changed
in the future.

### The LK201 Keyboard


The escape key, used in all *META* commands, is very poorly placed on
the LK201 keyboard. To make typing a bit easier, the grave accent is also
considered to be a *META* prefix on the LK201;
the grave accent is right where you expect the escape key to be located.
A grave accent must be quoted to exter it into the text.

The arrow keys and all of the keys on the keypad
work just like they do on the VT-100 keyboard.

The keys immediately above the arrow keys try to do what their name
implies. `Next Screen` is the same as **C-V**.
`Prev Screen` is the same as
**M-V**. `Select` is the same as **C-@ ** (set mark).
`Remove` is the same as
**C-W** (kill region). `Insert here`
is the same as **C-Y** (yank from killbuffer).
`Find` is the same as **C-S** (search forward).

The `F6`, `F7`, `F8`,
`F9`, `F10`, and `F14` keys are unused.
The `F11` is escape, which is a *META* prefix key.
The `F12` key is backspace. The `F13` key is linefeed.

The `Help` key is unused. The `Do`
key is the same as **C-X E** (execute keyboard macro).

The `F17` key is the same as **C-X P** (previous window).
The `F18` key is the same as **C-X N** (next window).
The `F19` key is the same as **C-X Z** (grow window).
The `F20` key is the same as **C-X C-Z** (shrink window).

## Key Bindings


Normally when you type a key, MicroEMACS attempts to
interprets the key as a command.  Internally, MicroEMACS keeps a table
of each possible key, and the command assocated
with that key.  This association is called the "key binding."  
Even text keys,
such as the letters, numbers, and punctuation, are "bound" to a command
called "ins-self"
that just inserts the key that invoked it into the text.

Not all commands are bound to a particular
key.  These commands can still be entered by using
the (**M-X**) (`ESC X`) "extended-command"
prefix, or by binding them to a key
with the "bind-to-key" command.
See the section [**Key Binding Commands**](#key-binding-commands)
below for more information.

In subsequent sections, the command descriptions will give not only the
key that the command is normally bound to, but also the command name,
which is useful when you want to change the key bindings.

## The Echo Line


The echo line has two purposes; displaying messages and asking one
line questions.

Two types of messages are displayed in the echo line.
Informational messages tell you something useful, but do not imply in any
way that something is wrong. These messages are always enclosed in square
brackets ([ and ]).
Error messages indicate that something has
prevented a command from being executed. They are never enclosed in square
brackets, and will be accompanied by a beep from the terminal's bell if
the sound of the bell is not too obnoxious.

The echo line is cleared by the next keystroke.

The echo line is also used for asking and answering questions.
After the
prompt, you can type any characters you like. The reply is always
terminated by a `Return`. Before you commit to the reply, you can edit the reply
using the following characters:

* `Backspace` or `Rubout`: delete the character to the left of the cursor.

* `Control-A`: move the cursor to the beginning of the line.

* `Control-B`: move the cursor one character to the left.

* `Control-D`: delete the character under the cursor.

* `Control-E`: move the cursor to end of the line.

* `Control-F`: move the cursor one character to the right.

* `Control-K`: delete from the cursor to the end of the line.

* `Control-Q`: enter the next character literally into the line (useful for entering control characters).

* `Control-U`: delete the entire line.

You can also abort the command in progress by typing `Control-G`.
Command processors are designed to ask all questions before doing nasty
things, so that you will never do damage by aborting a command.

The echo line supports autocompletion.  If you are entering
a command name, a filename, or a buffer name, you can use the
`Space` or `Tab` keys to tell MicroEMACS to complete as
much of the entry as possible, based on what you have entered so far.
If you are entering a filename, pressing `Control-S` will fill in
the directory part of the current buffer's filename, making it easier
to find a file in the same directory.
Pressing the `?` or `Control-D` keys will open a new temporary window containing
the possible list of choices.

If you are entering a search string, pressing `Control-S` will fill in
the previous search string.

## Command Arguments

All commands can be given a numeric argument.
Most commands use this
argument as a repeat count. Commands can tell if an argument has been
supplied, and will use a default argument if no user argument is present.
Usually, the default argument is `1`.

A **C-U** preceding a command always introduces an argument.

If a numeric argument is just a string of **C-U** characters, then the
value of the argument is
4^(number of **C-U** characters), where "^" means "to the power."
Therefore **C-U** is 4, **C-U C-U** is 16,
**C-U C-U C-U** is 256, and so on.
Some commands
care if an argument is present, but don't look at its value; the **C-U**
character is a good way to say "here is an argument".

If there is a signed decimal number after the string of
**C-U** characters it specifies the value of the argument. In this case
the number of **C-U** characters does not matter. An argument of 10 can be
represented by **C-U 10**,
**C-U C-U 10**, or even **C-U C-U C-U C-U 10**.

## Dot, Mark, and the Region

Most editing commands act on the character or characters surrounding
the current location in the current buffer.  The thing that marks the current
location is always called *dot*.  The dot always points between two
characters.  This isn't the way that the cursor works on most display terminals,
so, by convention, dot is always immediately to the left of the character
on which the hardware cursor is placed.

There is also a second remembered position in the buffer, called the
*mark*. There are commands that set the value of mark.

Some commands act on a block of text called the *region*.
The region is
all of the characters between dot and mark. The relative positions of dot
and mark do not matter.

