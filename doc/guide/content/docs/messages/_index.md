---
weight: 16
bookFlatSection: true
title: "Messages"
---

# Messages


MicroEMACS has a message system that allows commands to display
error or status messages
that are too long to
fit on the echo line.  At present, only
the **display-version** command uses the message system, but other commands
may use it in the future.

[unbound]

:   **display-message**

    Display the message lines one at a time on the echo line, then enter
    a special message display mode, in which certain keys have the following
    meanings:

    * **C-N, Space, Return**: go forward one line, and quit if the end of
    the message has been reached.

    * **C-P, C-H, Backspace**: go backward one line.

    * **C-G**: quit, leave the message unchanged.

    * **C-C**: quit, erase the portion of the message already read, but leave
    the remainder of the message unchanged.

M-C-V

:   **display-version**

    Copy the version strings into the message buffer, then execute
    a **display-message** command (see above).  Use this command to
    find out which version of MicroEMACS you are running.

