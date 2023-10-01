---
weight: 13
bookFlatSection: true
title: "Keyboard Macros"
---

# Keyboard Macros



Keyboard macros simplify a large class of repetitious editing tasks.
The basic idea is simple. A set of keystrokes can be collected into a
group, and then the group may be replayed any number of times.

There is only one keyboard macro.  However, you can effectively have
more than one macro by giving the current keyboard macro a name.
Otherwise, when you define a new keyboard macro, the old one is erased.

You can also save macros for use in future editing sessions, with
the ins-macro function.

C-X (

:   **start-macro**

    This command starts the collection of a keyboard macro. All
    keystrokes up to the next **C-X )** will be gathered up,
    and may be replayed by
    the execute keyboard macro command.

C-X )

:   **end-macro**

    This command stops the collection of a keyboard macro.

C-X E

:   **execute-macro**

    The execute keyboard macro command replays the current keyboard
    macro. If an argument is present, it specifies the number of times the macro
    should be executed. If no argument is present, it runs the macro once.
    Execution of the macro stops if an error occurs (such as a failed
    search).

[unbound]

:   **name-macro**

    This command saves the current keyboard macro and gives it a name.
    MicroEMACS prompts you for the name to assign to the macro.  The macro name
    can then be used as an extended command
    (using `ESC X`),
    or can be bound to a key using
    **bind-to-key**, just as a normal command.

[unbound]

:   **ins-macro**

    This commands inserts the contents of a macro into the current
    buffer in a format that can be used later in a profile.
    MicroEMACS prompts for the name of the macro.  If you don't enter a name,
    the current macro is used.

