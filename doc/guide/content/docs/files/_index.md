---
weight: 12
bookFlatSection: true
title: "Files"
---


# Files



C-X C-F

:   **set-file-name**

    This command prompts in the echo line for a file name,
    which becomes the new associated file name for the current buffer.

C-X C-R

:   **file-read**

    This command prompts in the echo line for a file name,
    then it deletes all of the text in the current buffer and reads in the
    file. The associated file name is set to the name of the file just
    read. The number of lines read is displayed in the echo line.

C-X C-I

:   **file-insert**

    This command prompts in the echo line for a file name,
    then reads in the
    file, inserting its contents at the dot in the current buffer.
    The number of lines read is displayed in the echo line.
    The file name associated with the current buffer is not changed.

C-X C-S, M-T

:   **file-save**

    This command writes the contents of the current buffer
    to its associated file. The "changed" flag for the current buffer
    is
    reset. It is an error to use this command in a buffer which lacks
    an associated file name. This command is a no-operation if the
    buffer has not been changed since the last write.

    If you used the **-b** option when you invoked MicroEMACS, and
    this is first time a **file-save** command has been performed
    on the file, MicroEMACS will create a backup of the file.
    See the section [**Starting**](#starting) for more information on backups.

    The **C-X C-S** form of the command is not usable if the terminal being used
    required XON/XOFF support.
    In fact, if you use this format of the command
    on such a terminal, it will hang until you type **C-Q**.

    The **M-T** form of this command is easily entered on the
    Z-29 keyboard
    by pressing the `F2` key.  On PCs, this function is also bound to `F2`.

C-X C-V

:   **file-visit**

    This command selects a file for editing. It prompts for
    a file name in the echo line. It then looks through all of the buffers
    for a buffer whose associated file name is the same as the file being
    selected. If a buffer is found, it just switches to that buffer.
    Otherwise it creates a new buffer, (fabricating a name from the last
    part of the new file name), reads the file into it, and switches to the
    buffer.

    If the desired new buffer name is not unique (perhaps you tried to
    visit a file in some other directory with the same name as a file already
    read in) the command will prompt for a new buffer name. You can either
    supply a buffer name, or just type newline to overwrite the old
    buffer.

    On PCs, this function is also bound to `F3`.

C-X C-W

:   **file-write**

    This command prompts in the echo line for a file name,
    then it writes the contents of the current buffer to that file. The
    "changed" flag for the current buffer is reset, and the supplied file
    name becomes the associated file name for the current buffer.

