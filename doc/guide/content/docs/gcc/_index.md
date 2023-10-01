---
weight: 19
bookFlatSection: true
title: "GCC Errors"
---

# GCC Errors


MicroEMACS is able to parse a file containing gcc error messages, and
load the source files mentioned in the error messages.  Typically, you would
save the error messages to a file when running `make`:

    make >&errs

Then load the resulting `errs` file into MicroEMACS and use
the following command to process the error messages contained within:

M-C-E

:   **gcc-error**

    This command looks for the next gcc error message in the current buffer.
    If one is found, then a new window is opened if there is currently only
    one window; otherwise the next window below the current one is used.
    MicroEMACS reads into that window the file indicated in
    the error message, positions the cursor at the line and column given
    in the error message, and displays the remainder of the error message
    in the echo line.  To find the next error location, switch
    back to the error buffer (using the `back-window` command (**C-X P**)
    or the `forw-window` command (**C-X N**)),
    and issue this command again.

