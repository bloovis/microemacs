---
weight: 21
bookFlatSection: true
title: "Profiles"
---

# Profiles


A profile is a file that contain a sequence of characters that
MicroEMACS can read as if the file were a keyboard.  Thus
a profile is similar to a macro, but because it is a file
it doesn't go away when you leave MicroEMACS.  You can think of a profile as
an editor command file.

When MicroEMACS starts,
it reads and executes a startup profile, after it reads the
file(s) you specify on the command line (if any).  You might use the
startup profile to set up some favorite key bindings,
or define a macro,
or read in more files.

You can use the **-p profile** option when you invoke MicroEMACS to specify
the name of the startup profile.  If you don't use this option, MicroEMACS
will use a default profile name.
The name of the default
profile is `PE.PRO` on all systems except UNIX, where it is
named `.pepro`.  MicroEMACS will look for this profile in the
current directory.  If the profile is not found, MicroEMACS will
look in a second directory, as follows:

* On UNIX and PC-DOS,
the directory indicated by the environment string
**HOME**, if defined.

* On FlexOS, the **home:** directory or device.

* On CP/M-68K, user area 0 of the current drive.

* On VMS, the **sys$login** directory.

A profile consists of a series of tokens separated by white space
or newlines.  A token can be a literal string, key name, or command
name.

1.  **Literal string:** a series of characters surrounded by double quotes (").
    The characters within the quotes are interpreted by MicroEMACS exactly
    as if you had typed them on the keyboard.  Certain control characters
    may be placed in the string using the following escape sequences:

    \\n

    :   linefeed

    \\t

    :   tab

    \\b

    :   backspace

    \\r

    :   carriage return

    \\f

    :   form feed

    \\\\

    :   backslash

    \\"

    :   double quote

    Other control characters can be placed in the string by preceding
    them with the Control-Q (quote) character.

    A quoted string must always follow a command that normally
    prompt the user to enter a response string (for example, the search command
    **C-S**).  The
    last character in the response string must be a carriage return
    (written as \\r).

2.  **Key name:** the name of a MicroEMACS key
    surrounded by brackets [].
    Key names use the conventions established in this manual.  Examples:

    [C-F]

    :   means "Control-F"

    [M-C-V]

    :   means "ESC Control-F"

    [M-L]

    :   means "ESC L"

    [C-X E]

    :   means "Control-X E"

    [C-X C-S]

    :   means "Control-X Control-S"

    You can use the **display-bindings**
    extended command to get a partial
    list of key names, or see the [**Wall Chart**](#wall-chart) section below.

    MicroEMACS converts the key name to the corresponding internal key code.
    Only one key name is allowed within a single pair of brackets.

3.  **Commands name:** simply the name of any MicroEMACS command, whether
    it is bound to a key or not.  MicroEMACS prefixes the command name
    with **ESC X**, and follows it with a **Return**, before interpreting
    the command.  This simulates the keystrokes that you would enter at
    the keyboard to
    invoke an extended command.

4.  **Decimal number:** a series of digits, optionally preceded by a
    minus sign ("-").  It is equivalent to typing Control-U, followed
    by the number, on the keyboard.  Placing a number before
    a command is a convenient method of supplying a numeric argument
 to the command.

As an example, consider the following line from a profile:

    bind-to-key "help\r" [m-h]

This is equivalent the following key sequence typed at the keyboard:

    ESC X bind-to-key RETURN help RETURN ESC H

Note especially the '\\r' character in the quoted string: this must
be present
because MicroEMACS always expects you to type **Return**
in response to the `Function:` prompt of the **bind-to-key** command.
This is true for all commands that prompt on the echo line for a reply.

Here is a profile that moves the cursor down by 10 lines, and which
demonstrates the use of a numeric argument to a command:

    10 forw-line

Here is a profile that changes all occurrences
of the string "plugh" to "xyzzy" in the current file, then saves the
changes and quits.

    replace-string "plugh\r" "xyzzy\r" file-save quit

Here is a profile that causes the `Control-J` key
to indent according to Ruby conventions.

    bind-to-key "ruby-indent\r" [C-J]


You can automate this kind of global change with the **-p** option
when you invoke MicroEMACS.  If you name the above profile
`junk.pro`, you can perform a global change on a file,
without entering any MicroEMACS commands,
by invoking MicroEMACS with the following:

    pe -p junk.pro filename

You can tell MicroEMACS to read a profile at any time, with the
following command.

**C-X F**

:   **read-profile**

    This command prompts you for the name of a profile.
    MicroEMACS then reads its subsequent commands from the specified
    profile, until the end of the file is encountered.  While
    a profile is being processed, command
    errors are reported, but otherwise no screen activity takes place
    (unless the "echo" command is used in the profile).
    You cannot nest profiles by putting a [C-X F] or read-profile command in
    a profile.  Execution of a profile does *not* stop if an error occurs (such
    as a failed search).

You can display messages on the echo line during profile
processing (or at any other time) with the following command.

**C-X C-E**

:   **echo**

    This command prompts you to enter a line of text.
    MicroEMACS then displays the text on the echo line.
    This command might be useful for displaying progress
    during a time-consuming profile.  Here is an example
    of the use of the echo command in a profile:

        echo "Changing all CDOS references...\r"
        replace-string "CDOS\r" "FlexOS\r"

    Note the required carriage return (\\r) at the end of the string.

