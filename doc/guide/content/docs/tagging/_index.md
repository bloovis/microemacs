---
weight: 18
bookFlatSection: true
title: "Tags"
---

# Tags


MicroEMACS can use the source-code tagging programs `cscope` and `ctags`
to make it easier to find functions and variables in C programs.  The `find-` functions
prompt for a name to find, but they also attempt to extract a name from the
current buffer and location, to be used if you do not enter a name and simply hit
`Enter`.

M-,

:   **find-cscope**

    This command prompts for an identifier, then uses `cscope` to find the first
    occurence of the identifier.  Usually, the first occurrence is the one
    that defines the identifier.  The command then visits the corresponding
    file and places the dot at the line containing the identifier.  On PCs,
    this function is also bound to `F11`.

M-G

:   **find-grep**

    This command prompts for a string, then uses `cscope`'s "grep" function to find the first
    occurence of the string.  The command then visits the corresponding
    file and places the dot at the line containing the string.

[unbound]

:   **next-cscope**

    This command uses `cscope` finds the next occurrence of an identifier that
    was found in a previous `find-cscope` or `find-grep` command.  On PCs, this
    function is bound to `F12`.

M-.

:   **find-tag**

    This command prompts for an identifier, then reads the `TAGS` file (generated
    by `ctags`) to find an occurrence of the identifier.  If no argument is present,
    the command finds the first occurrence; if an argument is present, the command
    finds the next occurence.  The command then visits the corresponding
    file and places the dot at the line containing the identifier.

