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
  "rb_errinfo",
  "rb_set_errinfo",
  "rb_intern2",
  "rb_funcall",
  "rb_str_new",
  /* End of API names */
};

/* C functions callable from Ruby. */

/*
 * Look up an underscore-ized symbol in the symbol table.
 * Underscores in the symbol name are converted to dashes
 * before the lookup is performed.
 */
static SYMBOL *
rubylookup (const char *s)
{
  char *s1 = malloc (strlen (s) + 1);
  char *p;
  char c;
  SYMBOL *sp;

  if (s1 == NULL)
    {
      eprintf ("Out of memory in ruby.c");
      return NULL;
    }
  for (p = s1; (c = *s) != '\0'; ++p, ++s)
    {
      if (c == '_')
	*p = '-';
      else
	*p = c;
    }
  *p = '\0';
  sp = symlookup (s1);
  free (s1);
  return sp;
}


/*
 * Check if the string c is is actual MicroEMACS command.
 */
static VALUE
my_iscmd (VALUE self, VALUE c)
{
  VALUE ret;
  const char *name;

  ret = Qfalse;
  if (RB_TYPE_P(c, T_STRING))
    {
      name = StringValueCStr(c);
      if (rubylookup (name) != NULL)
	ret = Qtrue;
    }
  return ret;
}

/*
 * Run a MicroEMACS command.  Return its result code
 * as a FIXNUM (0 = FALSE, 1 = TRUE, 2 = ABORT).
 *
 * FIXME: pass optional strings so that they can be used
 * by eread.
 */
static VALUE
my_cmd (VALUE self, VALUE c, VALUE f, VALUE n, VALUE k, VALUE s)
{
  VALUE ret;
  const char *name;
  int flag;
  int narg;
  int key;
  int cret = TRUE;
  SYMBOL *sp;

  if (RB_TYPE_P(c, T_STRING))
    name = StringValueCStr (c);
  else
    {
      eprintf ("command name must be a string");
      cret = FALSE;
    }

  if (FIXNUM_P(f))
    flag = FIX2INT (f);
  else
    {
      eprintf ("flag must be a fixnum");
      cret = FALSE;
    }

  if (FIXNUM_P(n))
    narg = FIX2INT (n);
  else
    {
      eprintf ("numeric argument must be a fixnum");
      cret = FALSE;
    }

  if (FIXNUM_P(k))
    key = FIX2INT (k);
  else
    {
      eprintf ("key must be a fixnum");
      cret = FALSE;
    }

  if (!RB_TYPE_P(s, T_ARRAY))
    {
      eprintf ("strings must be an array");
      cret = FALSE;
    }
  else
    {
      /* Collect the strings in the array and add
       * them to the reply queue for eread.
       */
      int ci;
      VALUE len = rb_funcall (s, rb_intern("length"), 0);
      int clen = FIX2INT (len);
      for (ci = 0; ci < clen; ci++)
	{
	  VALUE i = LONG2FIX (ci);
	  VALUE str = rb_funcall (s, rb_intern ("slice"), 1, i);
	  char *cstr = StringValueCStr (str);
	  replyq_put (cstr);
	}
    }

  /* If all parameters look OK, run the command.
   */
  if (cret == TRUE)
    {
     cret = FALSE;

      /* fprintf (stdout, "my_cmd: name = %s\n", name); */
      if ((sp = rubylookup (name)) != NULL)
	{
	  if (sp->s_macro)
	    {
	      if (kbdmip != NULL || kbdmop != NULL)
		{
		  eprintf ("Not now");
		  cret = FALSE;
		}
	      else
		cret = domacro (sp->s_macro, 1);
	    }
	  else
	    cret = sp->s_funcp (flag, narg, key);
	}
      else
	{
	  eprintf ("Unknown command %s", name);
	  cret = FALSE;
	}
    }
  ret = INT2NUM (cret);
  return ret;
}

/*
 * Get the current line.
 */
static VALUE
my_getline (VALUE self)
{
  VALUE ret;

  ret = rb_str_new ((char *) lgets (curwp->w_dot.p), llength (curwp->w_dot.p));
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

  /* Define global functions that can be called from ruby.
   */
  rb_define_global_function("cmd", my_cmd, 5);
  rb_define_global_function("iscmd", my_iscmd, 1);
  rb_define_global_function("getline", my_getline, 0);
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
  char line[NCOL];

  if ((status = loadruby ()) != TRUE)
    return status;

  /* Ruby goes here */
  if ((status = ereply ("Ruby code: ", line, sizeof (line))) != TRUE)
    return status;
  rb_eval_string_protect(line, &state);
  if (state)
    {
      /* handle exception */
      VALUE exception = rb_errinfo ();	/* get last exception */
      if (RTEST(exception))
	{
	  VALUE msg = rb_funcall (exception, rb_intern("to_s"), 0);
	  eprintf ("ruby exception: %s", StringValueCStr (msg));
	}
      rb_set_errinfo (Qnil);		/* clear last exception */
    }
  return TRUE;
}
