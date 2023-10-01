---
weight: 9
bookFlatSection: true
title: "Paragraphs"
---

# Paragraphs


MicroEMACS has commands that deal with paragraphs.  A paragraph
is a sequences of lines separated by one or more of the following:

* Blank lines.

* Lines beginning with a space or tab character.

* Lines beginning with the "@" character (SCRIBE, FinalWord, and Sprint
  command lines).

* Lines beginning with the "." character (NROFF and PROFF command lines).

There are commands for "filling" a paragraph, moving dot to
the start or end of a paragraph, setting the fill column,
and performing automatic word wrap.

[unbound]

:   **set-fill-column**

    This command sets the current fill column to its argument
    (remember that the argument is entered as `Control-U` and a
    decimal number preceding the command).  If no argument is present,
    the column of the current location of the cursor (the dot) is used instead.
    The fill column is used by the **fill-paragraph**
    and **ins-self-with-wrap** commands.
    The default fill column is 70.

M-[

:   **back-paragraph**

    This command moves the dot to the beginning of the current paragraph.
    If the dot is not in a paragraph when this command is entered,
    it is moved to the beginning of the preceding paragraph.
    If an argument is provided, the dot is moved by that many paragraphs.

    If you are using MicroEMACS with a serial terminal, you may have
    to type `ESCAPE` followed by two `[` characters to invoke this command.
    The reason is that ESCAPE-[ is the prefix produced by
    function keys on VT-100 compatible terminals.

M-]

:   **forw-paragraph**

    This command moves the dot to
    the end of the current paragraph (actually to first separator line after
    the paragraph).
    If the dot is not in a paragraph when this command is entered,
    it is moved to the end of the following paragraph.
    If an argument is provided, the dot is moved by that many paragraphs.

M-J

:   **fill-paragraph**

    This command "fills" the current paragraph.  It inserts or
    deletes spaces and line breaks between words as needed to cause
    the text of each line to be filled out to (but no farther than)
    the current fill column.  The text is thus filled, but not
    right-justified.  The dot is then placed at the end of the
    last line of the paragraph.

M-C-W

:   **kill-paragraph**

    This command deletes the current paragraph.  If an argument is
    provided, that many paragraphs are deleted.

[unbound]

:   **ins-self-with-wrap**

    This command inserts the key that invoked it into the buffer
    at the current location of dot, and dot moves 1 character to the right.
    Then, if dot has gone past the current fill column,
    a line break is
    inserted in front of the first word in the line
    that extends past the fill column.  Thus the current line is
    "filled", and a new line is created that contains the words from
    the current line that wouldn't fit within the fill column.

    Normally, this command
    is not bound to any key.  If you bind this command to
    the space key, the effect will be similar to the word-wrapping mode
    in MINCE.

