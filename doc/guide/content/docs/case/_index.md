---
weight: 10
bookFlatSection: true
title: "Case Conversion"
---

# Case Conversion


In addition to the word mode case conversion commands, MicroEMACS has
commands to modify the case of large blocks of text. These commands should
be used with caution because they cause major damage to (potentially)
large areas of the buffer.

C-X C-L

:   **lower-region**

    The lowercase region command converts all of the characters
    between dot and mark into lower case.

C-X C-U

:   **upper-region**

    The uppercase region command converts all of the characters
    between dot and mark into upper case.
