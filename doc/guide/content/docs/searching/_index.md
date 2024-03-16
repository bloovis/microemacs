---
weight: 11
bookFlatSection: true
title: "Searching and Spelling"
---

# Searching and Spelling


Search commands move though the buffer, in either the forward or the reverse
direction, looking for text that matches a search pattern. Search commands
prompt for the search pattern in the echo line. The search pattern used
by the last search command is remembered, and displayed in the prompt. If
you want to use this pattern again, just hit carriage return at the
prompt.

In search strings, all characters stand for themselves, and all searches
are normally case insensitive.  The case insensitivity may be
defeated with the "fold-case"
command, described below.
The newline characters at the ends of the lines are
considered to have hexadecimal value 0A, and can be matched by a linefeed
(**Control-J**) in the search string.

A carriage return can be searched
for by preceding it with `Control-Q` in the search string.
On PC-DOS
this will not match the carriage return in the CR-LF character pair
that normally terminates a line of text.  It will only match a bare
carriage return that has no following line feed.

MicroEMACS supports regular expression
searches using a subset of POSIX regular expressions:

* `.` (dot) matches any character

* character classes (square brackets with optional ^ negation operator)

* groups (parentheses)

* alternatives (`|`)

* `^` (start of line) and `$` (end of line)

* `?` (zero or one occurrence)

* `*` (zero or more occurrences)

* `+` (one or more occurrences)

C-S, M-S

:   **forw-search**

    Search forward, from the current location, toward the end of
    the buffer. If found, dot is positioned after the matched text. If the
    text is not found, dot does not move.

    The **C-S**
    form of the command is not usable if the terminal being used
    required XON/XOFF support.
    In fact, if you use this format of the command
    on such a terminal, it will hang until you type **C-Q**.

    The **M-S** form of this command is easily entered on the Z-29
    keyboard
    by pressing the `F1` key.

C-R

:   **back-search**

    Search reverse, from the current location, toward the front of
    the buffer. If found, dot is positioned at the first character of the
    matched text. If the text is not found, dot does not move.

M-C-S

:   **forw-regexp-search**

    Similar to **forw-search**, except that the search string is
    a regular expression, and searches cannot cross line boundaries.

M-C-R

:   **back-regexp-search**

    Similar to **back-search**, except that the search string is
    a regular expression, and searches cannot cross line boundaries.

M-C-F

:   **fold-case**

    Enable or disable case folding in searches, depending on the
    argument to the command.  If the argument is zero, case folding
    is disabled, and searches will be sensitive to case.  If the
    argument is non-zero, case folding is enabled, and searches
    will NOT be sensitive to case.

M-P

:   **search-paren**

    Search for a match of the character at the dot.  If the character
    is a parenthesis or bracketing character, move the dot to the matching
    parenthesis or bracket, taking into account nesting and C-style
    comments.  A parenthesis or bracketing character is one of the
    following: (){}[]<>

Pad-1,Shift-5

:   **search-again**

    Repeat the last forw-search command
    (**C-S, M-S**) or prev-search command (**C-R, M-R**),
    without prompting for a string.  This command is bound to two keys
    on the numeric keypad of the Z-29
    terminal: the `1` key, and the shifted
    `5` key.
    On PCs, this function is also bound to `F9`.

C-X S

:   **forw-i-search**

    Enters incremental search mode, with
    the initial search direction being forward.  In incremental search mode
    the following keys are recognized.

    * **C-N** finds the next occurence of the string (if it is first thing typed,
    reuse the previous string).

    * **C-P** finds the previous occurence of the string
    (if it is the first thing typed, reuse the previous string).

    * **C-S** (or **C-F**) switches the search direction to forward,
    and finds the next occurrence
    of the string.

    * **C-R** (or **C-B**) switches the search direction to reverse,
    and finds the next occurrence
    of the string.

    * **C-Q** (or **C-^**) quotes the next character (allows searching for C-N, etc.).

    * **ESC** exits from incremental search mode.

    * **C-G** restores dot to original location before incremental search mode
    was entered, then exits from incremental search mode.

    * **DEL** undoes the effect of the last character typed in the search string.

    * **C-U**, **C-X**, **C-J**, and all non-Control
    characters accumulate into search string.

    * All other control characters exit from incremental search mode and
    are interpreted as normal commands.

C-X R

:   **back-i-search**

    Enters incremental search mode, with the initial search direction
    being reverse.
    Otherwise identical to **C-X S**.

M-Q,M-%

:   **query-replace**

    Search and replace with query.  This command prompts for
    a search string and a replace string, then searches forward for the
    search string.  After each occurrence of the search string is found,
    the dot is placed after the string, and
    the user is prompted for action.  Enter one of the following characters:

    * **space** or **,** (comma) causes the string to be replaced, and the
    next occurrence is searched.

    * **.** (period) causes the string to replaced, and quits the search.

    * **n** causes the string to be skipped without being replaced, and
    the next occurrence is searched.

    * **!** causes all subsequent occurrences of the string to be replaced
    without prompting.

    * **Control-G** quits the search without any further replacements.

    Normally this command adjusts the capitalization of the new string to
    match the old string when it performs a replacement.
    You can defeat this "feature"
    if you prefix this command with an argument (the argument value is ignored).

M-R

:   **replace-string**

    Prompt for a search string and a replacement string,
    then search forward for all occurrences of the search string, replacing
    each one with the replacement string.  Do
    not prompt the user for
    confirmation at each replacement, as in the **query-replace** command.

    Normally this command adjusts the capitalization of the new string to
    match the old string when it performs a replacement.
    You can defeat this "feature"
    if you prefix this command with an argument (the argument value is ignored).

M-/

:   **reg-replace**

    Similar to **query-replace**, except that the search string is a
    regular expression, and the replacement string can contain
    the following special characters:

    * `&` stands for the entire matched string.

    * `\n`, where `n` is a digit in the range 0-9, stands for the nth
      matched group (where groups are delineated by parentheses in the
      regular expression pattern).

    * `\` followed  by either `\` or `&` stands for that character itself,
      without the leading `\`.

M-?

:   **rep-query-replace**

    Similar to **reg-replace**, except that the user is prompted
    to confirm each replacement, as in **query-replace**.

C-X I

:   **spell-region**

    This command uses `ispell` to spell-check the current region
    (the text between the mark and the dot).  At each misspelled
    word, MicroEMACS prompts for an action:

    * **q** or **C-G** aborts the spell checking.

    * **Space** ignores the misspelled word

    * **a** ignores the misspelled word and adds it to ispell's list of
    words to ignore in the future.

    * **0** to **9** replaces the misspelled word with one of up to ten
    suggestions; the suggestions are shown in the prompt on the echo line.

    * **r** prompts for a string to replace the word.

M-\$

:   **spell-word**

    Similar to **spell-region**, except that it checks only the word under
    the cursor.

