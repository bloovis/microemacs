---
weight: 20
bookFlatSection: true
title: "Undo"
---

# Undo

MicroEMACS supports an undo/redo facility.  It saves
information about the last 100 commands that modified a given buffer,
so that these commands can be undone.  Each buffer has its own set of
undo records.  MicroEMACS treats consecutive typing of normal (non-command)
keys as a single operation; this makes it less tedious to undo large
amounts of typing.

The most recent undo operation(s) can be reversed with the `redo`
command.  By using `undo` and `redo` in succession, you can go
backwards and forwards in time through the recent history of a buffer.
But if you modify the buffer in any way other than through `undo` or
`redo`, the ability to `redo` will be lost until another `undo` is
performed.  This prevents conflicting changes from being made
to a buffer.

C-X U

:   **undo**

    This command undoes the most recent operation that modified the current buffer.
    On PCs, this function is also bound to `F5`.

[unbound]

:   **redo**

    This command undoes the most recent undo.  On PCs, this function is bound to `F7`.

