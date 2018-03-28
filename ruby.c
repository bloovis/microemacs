/*
    Copyright (C) 2018 Mark Alexander

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

#include	"def.h"

#include	<dlfcn.h>
#include	<ruby.h>

static void *ruby_handle;
void *ruby_fptrs[100];

/*
 * Do not change the comments in the following table.  They
 * are used by makeapi.rb script to generate the trampoline code
 * for the Ruby APIs.
 */
const char *fnames[] =
{
  /* Start of API names */
  "ruby_setup",
  "ruby_cleanup",
  "rb_eval_string_protect",
  "rb_define_global_function",
  "rb_string_value_cstr",
  "rb_fix2int",
  "rb_num2int",
  /* End of API names */
};

/* C functions callable from Ruby. */

static VALUE
my_echo (VALUE self, VALUE arg1)
{
  VALUE ret;

  if (RB_TYPE_P(arg1, T_STRING))
    eprintf ("%s", StringValueCStr(arg1));
  else
    eprintf ("Arg type to echo is not string");
  ret = INT2NUM (0);
  return ret;
}


/* Load the ruby library and initialize the pointers to the APIs.
 * Return TRUE on success, or FALSE on failure.
 */
static int
loadruby (void)
{
  int i, status;

  if (ruby_handle != NULL)
    return TRUE;
  ruby_handle = dlopen("libruby-2.3.so", RTLD_LAZY);
  if (ruby_handle == NULL)
    {
      eprintf ("Unable to load ruby libraryn");
      return FALSE;
    }
  for (i = 0; i < sizeof (fnames) / sizeof (fnames[0]); i++)
    {
      ruby_fptrs[i] = dlsym (ruby_handle, fnames[i]);
      if (ruby_fptrs[i] == NULL)
	{
	  eprintf ("Unable to get address of ruby function %s", fnames[i]);
	  return FALSE;
	}
      else
	{
	  /* printf("Address of %s is %p\n", fnames[i], ruby_fptrs[i]); */
	}
    }
  if ((status = ruby_setup ()) != 0)
    {
      eprintf ("ruby_setup returned %d", status);
      return FALSE;
    }

  /* Define a global function called "echo". */
  rb_define_global_function("echo", my_echo, 1);

  return TRUE;
}

#if 0
static void
unloadruby (void)
{
  int status;

  /* destruct the VM */
  status = ruby_cleanup(0);
  if (status != 0)
    eprintf ("ruby_cleanup returned %d", status);
}
#endif

int
rubystring (int f, int n, int k)
{
  int status;
  int state;
  VALUE result;
  char line[NCOL];

  if ((status = loadruby ()) != TRUE)
    return status;

  /* Ruby goes here */
  if ((status = ereply ("Ruby code: ", line, sizeof (line))) != TRUE)
    return status;
  result = rb_eval_string_protect(line, &state);
  if (state)
    {
      /* handle exception */
      eprintf ("rb_eval_string_protect returned an exception");
    }
  else
    {
      /* Check the return code from echo */
      status = NUM2INT (result);
      if (status != 0)
	eprintf ("ruby code returned %d", status);
    }

  return TRUE;
}
