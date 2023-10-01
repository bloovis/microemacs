---
weight: 6
bookFlatSection: true
title: "Inserting"
---

# Inserting



All characters between hexadecimal 20 and 7E (blank through tilde),
and all characters between hexadecimal A0 and FE (right hand graphics)
are self-inserting. They are inserted into the buffer at the current
location of dot, and dot moves 1 character to the right.

The **Tab**) key is also self-inserting.
By default, MicroEMACS has fixed tab settings at columns 9, 17, 25, and so on,
but you can change the tab width using the **set-tab-size** command.

Any self-inserting character can be given an
argument.
This argument is
used as a repeat count, and the character is inserted that number of
times. This is useful for creating lines of `*` characters of a specific
length, and other pseudo-graphic things.

Self-inserting keys are all bound to the
function **ins-self**.  As with
any key, these keys can be rebound to other functions, but this is not
always advisable.

Word wrapping can be performed by binding a key
to the **ins-self-with-wrap** function.
This function is described in the [Paragraphs section]({{< relref "paragraphs" >}}).

C-M, Return

:   **ins-nl**

    The Return key works just like you would expect it to work;
    it inserts a newline character. Lines can be split by moving into the middle
    of the line, and inserting a newline.

    On some terminals with slow displays,
    if dot is positioned at end of line,
    and the line after the current line is a blank line,
    then **C-M** does not insert a newline,
    but simply moves to the first position
    on the following line. This lets you create a block of blank space (perhaps
    using **C-O** and then type text into it.  This "feature" can be enabled
    at compile-time by
    setting the "NLMOVE" definition to 1 in `def.h`.

[unbound]

:   **ins-nl-and-indent**

    This command is like a **C-M** with indentation.
    It inserts a new line, and then inserts enough tabs and spaces to
    duplicate the indentation of the previous line.
    This is useful for entering heavily-indented programs in structured
    languages like PASCAL or C.

[unbound]

:   **borland-indent**

    This command is like a **ins-nl-and-indent**, but also attempts to indent
    according to the coding standards in use at Borland in the 1990s.
    If the previous line starts with `{`, or an argument
    of four (i.e. a single `Control-U`) is specified, indent by four spaces. If an
    argument of 16 (i.e. two `Control-U`s) is specified, reduce indentation by four
    spaces.  Otherwise retain the same indentation.

C-J

:   **gnu-indent**

    This command is like a **ins-nl-and-indent**, but also attempts to indent
    according to the GNU coding standards in use at Cygnus in the 1990s.
    If the previous line starts with `{`, `if`, `while`, `for`, `else`, `case, or an argument
    of four (i.e. a single `Control-U`) is specified, indent by two spaces. If an
    argument of 16 (i.e. two `Control-U`s) is specified, reduce indentation by two
    spaces.  Otherwise retain the same indentation.

[unbound]

:   **vmware-indent**

    This command is like a **ins-nl-and-indent**, but also attempts to indent
    according to the coding standards in use at VMware in the 2000s.
    If the previous line starts with `{`, or an argument
    of four (i.e. a single `Control-U`) is specified, indent by three spaces. If an
    argument of 16 (i.e. two `Control-U`s) is specified, reduce indentation by three
    spaces.  Otherwise retain the same indentation.

[unbound]

:   **ruby-indent**

    This command is like a **ins-nl-and-indent**, but also attempts to indent
    according to commonly accepted Ruby conventions.
    If the previous line starts with `{` or one of the many block-start keywords, or an argument
    of four (i.e. a single `Control-U`) is specified, indent by two spaces. If an
    argument of 16 (i.e. two `Control-U`s) is specified, reduce indentation by two
    spaces.  Otherwise retain the same indentation.

C-O

:   **ins-nl-and-backup**

    This command creates blank lines. To be precise, it inserts
    a newline by doing a **C-M**,
    and then backs up by doing a **C-B**. If dot is at
    the start of a line, this will leave dot sitting on the first character
    of a new blank line.

C-Q, C-X Q

:   **quote**

    Characters which are special to MicroEMACS can be inserted by
    using this command.
    The next character after the **C-Q** or **C-X Q** is stripped of
    any special meaning. It is simply inserted into the current buffer.
    Any argument specified on the **C-Q**
    or **C-X Q** command is used as the insert
    repeat count.

    The **C-Q** form of the command is the easiest to use. However, some terminals
    demand that MicroEMACS perform XON/XOFF
    processing. If this is the case,
    the **C-Q** will be eaten by low level terminal support,
    and will not be usable
    as a command. The **C-X Q** form can always be used.

C-T

:   **twiddle**

    Exchange the two characters on either side of the dot.
    If the the dot is at the end of the line, twiddle the two characters
    before it.  Does nothing if the dot is at the beginning of the line.
    This command is supposedly useful for correcting common
    letter transpositions while entering new text.

M-Tab

:   **set-tab-size**

    Set the tab size to the value of the argument, which must be
    a positive number greater than 1 (the default tab size is 8).
    This only affects the how
    MicroEMACS displays tabs on the screen; it does not affect
    how tabs are saved when a file is written to disk.  To change
    how MicroEMACS handles tabs when saving a file, see the
    **set-save-tabs** command.

M-I

:  **set-save-tabs**

    By default, MicroEMACS preserves tabs when it writes
    a file to disk.  If you pass a zero argument to this
    command, MicroEMACS will convert tabs to spaces when
    writing a file; the number of spaces is determined
    by the tab size (which you can set using **set-tab-size**).
    If you pass a non-zero argument to this command,
    MicroEMACS will revert back to the default behavior,
    which is to preserve tabs.

[unbound]

:   **just-one-space**

    If the dot is currently sitting over a tab or a space, all consecutive
    tabs and spaces to the left and right of the dot are deleted.
    Then a single
    space is inserted, and the dot is moved 1 character to the right.
