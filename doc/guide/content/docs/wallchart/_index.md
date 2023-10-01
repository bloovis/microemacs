---
weight: 27
bookFlatSection: true
title: "Wall Chart"
---

# Wall Chart

Here is a list of the current key
bindings in MicroEMACS.  The
terminal-dependent key bindings
are presented at the end of this
section. 

## Insert and Delete

| Key | Command Name |
| --- | ------------ |
| Return | ins-nl |
| C-J | gnu-indent |
| C-O | ins-nl-and-backup |
| C-T | twiddle |
| M-L | lower-word |
| M-U | upper-word |
| M-C | cap-word |
| M-D | forw-del-word |
| C-D | forw-del-char |
| Rubout |  back-del-char |
| Backspace | back-del-char |
| M-Rubout | back-del-word |
| M-Backspace | back-del-word |
| C-K | kill-line |
| C-Y | yank |
| C-X C-O | del-blank-lines |
| M-C-U | unicode |
| M-Tab | set-tab-size |
| M-I | set-save-tabs |

## Little Moves

| Key | Command Name |
| --- | ------------ |
| C-F | forw-char |
| C-B | back-char |
| M-F | forw-word |
| M-B | back-word |
| C-N | forw-line |
| C-P | back-line |
| C-A | goto-bol |
| C-E | goto-eol |

## Big Moves

| Key | Command Name |
| --- | ------------ |
| C-V | forw-page |
| C-Z | back-page |
| M-V | back-page |
| M-< | goto-bob |
| M-> | goto-eob |
| C-X G | goto-line |


## Paragraphs

| Key | Command Name |
| --- | ------------ |
| M-[ | back-paragraph |
| M-] | forw-paragraph |
| M-J | fill-paragraph |
| M-C-W | kill-paragraph |

## Region Commands

| Key | Command Name |
| --- | ------------ |
| C-@,C-space | set-mark |
| M-. | set-mark |
| C-X C-X | swap-dot-and-mark |
| C-X C-L | lower-region |
| C-X C-U | upper-region |
| M-W | copy-region |
| C-W | kill-region |

## Search and Replace

| Key | Command Name |
| --- | ------------ |
| C-S | forw-search |
| M-S | forw-search |
| C-R | back-search |
| M-C-F | fold-case |
| M-P | search-paren |
| M-Q | query-replace |
| M-% | query-replace |
| M-R | replace-string |
| C-X S | forw-i-search |
| C-X R | back-i-search |
| M-C-S | forw-regexp-search |
| M-C-R | back-regexp-search |
| M-/ | reg-replace |
| M-? | reg-query-replace |

## File and System Operations

| Key | Command Name |
| --- | ------------ |
| C-X C-I | file-insert |
| C-X C-V | file-visit |
| C-X C-R | file-read |
| C-X C-W | file-write |
| M-T | file-save |
| C-X C-S | file-save |
| C-X C-F | set-file-name |
| C-X B | use-buffer |
| C-X K | kill-buffer |
| C-L | refresh |
| C-Q | quote |
| C-X Q | quote |
| M-X | extended-command |
| C-C | spawn-cli |
| C-X C-G | abort |
| C-G | abort |
| M-C-G | abort |
| C-X C-C | quit |
| C-X C | jeff-exit |
| C-X I | spell-check |

## Windows

| Key | Command Name |
| --- | ------------ |
| C-X 1 | only-window |
| C-X 2 | split-window |
| C-X + | balance-windows |
| M-! | reposition-window |
| C-X Z | enlarge-window |
| C-X C-Z | shrink-window |
| C-X N | forw-window |
| C-X O | forw-window |
| C-X P | back-window |
| C-X C-P | up-window |
| C-X C-N | down-window |
| M-C-R | display-message |
| M-C-V | display-version |
| C-X = | display-position |
| C-X C-B | display-buffers |
| C-X C-Q | toggle-readonly |

## Macros and Profiles

| Key | Command Name |
| --- | ------------ |
| C-X ( | start-macro |
| C-X ) | end-macro |
| C-X E | execute-macro |
| C-X F | read-profile |
| C-X C-E | echo |

## Tags, GCC

| Key | Command Name |
| --- | ------------ |
| M-, | find-cscope |
| M-G | find-grep |
| M-. | find-tag |
| M-C-E | gcc-error |

## PC Function Keys

| Key | Command Name |
| --- | ------------ |
| F1 | help |
| F2 | file-save |
| F3 | file-visit |
| F4 | quit |
| F5 | undo |
| F6 | display-buffers |
| F7 | redo |
| F8 | forw-buffer |
| F9 | search-again |
| F10 | only-window |
| F11 | find-cscope |
| F12 | next-cscope |
| Up | back-line |
| Down | forw-line |
| Left | back-char |
| Right | forw-char |
| PgUp | back-page |
| PgDn | forw-page |
| Home | goto-bol |
| End | goto-eol |
| C-Left | back-word |
| C-Right | forw-word |
| C-PgDn | down-window |
| C-PgUp | up-window |
| C-Home | goto-bob |
| C-End | goto-eob |
| Insert | yank |
| Delete | forw-del-char |
