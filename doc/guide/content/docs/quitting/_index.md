---
weight: 4
bookFlatSection: true
title: "Quitting"
---

# Quitting


When you finish editing, you have to quit.
MicroEMACS never writes out a file because it thinks that this
is the right thing to do. However, it will attempt to protect you from
forgetting to do so.


C-X C-C

:   **quit**

    This is the basic quit command. MicroEMACS exits, returning
    control to the shell. If there are any changed buffers that have not been
    written out, it will ask for permission to quit. Supplying an argument to
    **C-X C-C** makes the command quit unconditionally, without asking for
    confirmation. The value of the argument is not important.
    On PCs, this function is also bound to `F4`.

C-C

:   **spawn-cli**

    This command suspends the execution of MicroEMACS, and runs
    a command interpreter
    in a subjob.
    When the subjob terminates, the screen
    is cleared and repainted.

    Subjobs are implemented in FlexOS,
    PC-DOS
    VMS.
    Users of CP/M are out of luck.

    Exit the command interpreter and return to MicroEMACS with one of the following:

    * On VMS enter the **logout** command.

    * If you are using the c-shell
    on Unix-like operating systems, enter the **fg** command.

    * On all other systems enter the **exit** command.

C-X C

:   **jeff-exit**

    This is a slightly more elaborate quit command.
    If the current buffer has been changed, **C-X C** saves the contents of
    the buffer in the associated file (it gets an error if there is no
    associated file). If the current buffer has not been changed, **C-X C**
    acts like **C-C**.

