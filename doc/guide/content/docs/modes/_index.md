---
weight: 23
bookFlatSection: true
title: "Modes"
---

# Modes

If MicroEMACS has been built with Ruby support, it will also support
the notion of modes, which are similar to major modes in Emacs.
A mode consists of a name (which is arbitrary) and a set of key bindings
local to that mode, and is attached to a specific buffer.  By default,
buffers in MicroEMACS do not have modes, but you can provide support
for modes by loading Ruby support for them.  Modes can be used to
implement features for particular types of code, or to provide
special types of buffers that are not treated solely as plain text.

## Mode initialization

When MicroEMACS reads a file into a buffer, or visits a file (which
may or may not exist yet), it calls the Ruby function `initmode`, which
lives in the file `pe.rb`, and which gets loaded whenever MicroEMACS starts.
This function attempts to determine the name of the mode associated
with the file being edited.  First, it examines the first few lines
in the file itself for a line containing a string in the following format:

    -*-MODE-*-

where `MODE` is the name of the mode associated with the file.

If such a string cannot be found, `initmode` then examines the name
of the file itself, and attempts to find a match in the `$modetable`
array in `pe.rb`.  This table associates filename patterns with mode names.
The table has very few entries by default, but you can expand it as necessary by editing `pe.rb` directly,
or by overwriting it or adding to it in your own Ruby extension.

If `initmode` can determine the mode name, it calls the function `MODE_mode`,
where `MODE` is the name of the mode.  For example, if `initmode` determines
that the mode name is `c`, it calls the function `c_mode`, if it exists.
This function is called the mode hook, and it is responsible for performing
all necessary initialization for the mode.  If the mode hook does not exist,
MicroEMACS does not report an error.

## Simple mode example

To see how a rudimentary mode is written, look at the file `mode.rb`
in the MicroEMACS source code.  (This file should also have been installed
in `/usr/local/share/pe`, if you built MicroEMACS and ran `make install`.)

Here is the code for `c_mode`, the hook for C files, mentioned above:

    def c_mode
      setmode "C"
      bind "gnu_indent", ctrl('j'), true
      echo "This is C mode"
    end

The first line in this function creates a new mode for the current buffer,
and assigns it the name "C".  The name is arbitrary, and is used only
for displaying in the mode line for the buffer.  But `setmode` must
be called; otherwise a mode will not exist for the current buffer,
and attempts to create or use mode-local key bindings will not succeed.

The second line in this function binds the MicroEMACS function `gnu-indent`
to the **C-J** key.  This binding is local to this mode only.  If you switch
to a buffer that has a different mode, or no mode, the binding for **C-J** may well be different.

The third line in this function is present only for debugging purposes.
It can be safely deleted.

You can load this rudimentary mode support automatically by adding the following
line to `~/pe.rb` or `./pe.rb`:

    load 'mode.rb'

## A more complicated mode example

The file `dired.rb`, also provided with MicroEMACS, is a more elaborate example of a mode.
It implements a directory browser similar
to the dired mode in Emacs.  Unlike the simple
example shown above, it provides both global and mode-specific key bindings,
and it does not provide a mode hook function.
Instead, it provides a new command, `dired`, that is globally bound
to the keystroke **C-X D**:

    ruby_command "dired"
    bind "dired", ctlx('d')

The file `dired.rb` also creates three new commands but does not bind them to
keystrokes immediately:

    ruby_command "visitfile"
    ruby_command "openfile"
    ruby_command "displayfile"

The initialization of the mode happens in the `dired` function when
you enter the keystroke **C-X D**.  This function prompts you
for a directory name, then calls `showdir` to open a view
on the directory.

The `showdir` function opens a new window with a buffer called `*dired*`,
to avoid conflict with existing buffers.  It clears any existing contents
of the buffer.  It then runs `/bin/ls -laF` to
load the buffer with a directory listing.  The first line in the
buffer contains the directory name, and each subsequent line contains
information about a file in that directory, as provided by `ls`.  If the name of a file ends
in a '/' character, that file is actually a subdirectory.  Finally,
`showdir` marks the buffer as read-only.

The `showdir` function then performs some string matching to determine
that starting column for filenames in the directory listings.
Finally, it creates a mode for the directory listing and attaches
three key bindings to it:

    setmode "dired"
    bind "visitfile", ctrl('m'), true
    bind "openfile", key('o'), true
    bind "displayfile", ctrl('o'), true

The `bind` calls create key bindings for the three new commands
that were defined earlier.  These bindings perform three distinct
actions on the file under the cursor (called the "selected" file):

* The `Enter` key opens the selected file in a new window,
  replacing the current dired window (which still exists).

* The `o` key splits the screen into two windows, one containing
  the dired buffer, and the other containing the selected file.
  It then moves the cursor to the selected file.

* The `C-O` key is similar to the `o` key, except that it does
  not move the cursor to the selected file.

You can load the dired mode support automatically by adding the following
line to `~/pe.rb` or `./pe.rb`:

    load 'dired.rb'

