/*
    Copyright (C) 2008 Mark Alexander

    This file is part of MicroEMACS, a small text editor.

    MicroEMACS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
#define NSUBEXP  10
typedef struct regexp
{
  const char *startp[NSUBEXP];
  const char *endp[NSUBEXP];
  char regstart;		/* Internal use only. */
  char reganch;			/* Internal use only. */
  const char *regmust;		/* Internal use only. */
  int regmlen;			/* Internal use only. */
  char program[1];		/* Unwarranted chumminess with compiler. */
}
regexp;

extern regexp *regcomp (const char *exp);
extern int regexec (regexp * prog, const char *string);
extern int regsub (const regexp * prog, const char *source, char *dest, int destlen);
extern void regerror (const char *s);
