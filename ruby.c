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
#include	<unistd.h>

#define RUBY_DONT_SUBST
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
  "rb_str_new_static",
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
  static SYMBOL *sp;
  char *p;
  char c;
  char *s1 = strdup (s);

  /* Make a copy of s with '_' replaced with '-'.
   */
  if (s1 == NULL)
    {
      eprintf ("Out of memory in rubylookup!");
      return NULL;
    }
  for (p = s1; (c = *p) != '\0'; ++p)
    {
      if (c == '_')
	*p = '-';
    }

  /* If the previous symbol we found is the same, just
   * return that, instead of searching for it again.
   */
  if (sp == NULL || strcmp (s1, sp->s_name) != 0)
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
 * Helper function for Ruby bindtokey.  The first
 * parameter is a string containing the command
 * name, and the second parameter is the numeric
 * keycode.
 */
static VALUE
my_bindtokey(VALUE self, VALUE c, VALUE k)
{
  VALUE ret;
  const char *name = NULL;
  SYMBOL *sp = NULL;
  int key = KRANDOM;
  int cret = TRUE;

  if (RB_TYPE_P (c, T_STRING))
    {
      name = StringValueCStr (c);
      if ((sp = rubylookup (name)) == NULL)
	{
	  eprintf ("%s: no such command", name);
	  cret = FALSE;
	}
    }
  else
    {
      eprintf ("command name must be a string");
      cret = FALSE;
    }
  if (FIXNUM_P (k))
    key = FIX2INT (k);
  else
    {
      eprintf ("key must be a fixnum");
      cret = FALSE;
    }
  if (cret == TRUE)
    setbinding (key, sp);

  ret = INT2NUM (cret);
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
  const char *name = "";
  int flag = 0;
  int narg = 1;
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
 * Get the current line's text.
 */
static VALUE
my_getline (VALUE self)
{
  VALUE ret;
  VALUE utf8;

  ret = rb_str_new ((char *) lgets (curwp->w_dot.p), llength (curwp->w_dot.p));
  utf8 = rb_str_new_cstr("utf-8");
  rb_funcall (ret, rb_intern ("force_encoding"), 1, utf8);
  return ret;
}

/*
 * Get the current line number, 1 based so that it can
 * be used with goto-line, and for compatibility with
 * display-position.
 */
static VALUE
my_lineno (VALUE self)
{
  VALUE ret;

  ret = INT2NUM (lineno (curwp->w_dot.p) + 1);
  return ret;
}

/*
 * Get the current column number, 1 based for compatibility
 * with display-position.
 */
static VALUE
my_column (VALUE self)
{
  VALUE ret;

  ret = INT2NUM (curwp->w_dot.o + 1);
  return ret;
}

/*
 * Insert a string at the current location.
 */
static VALUE
my_insert (VALUE self, VALUE s)
{
  VALUE ret;
  int cret;

  if (!RB_TYPE_P(s, T_STRING))
    {
      eprintf ("insert parameter must be a string");
      cret = FALSE;
    }
  else
    {
      char *cs = StringValueCStr (s);
      cret = linsert (strlen (cs), 0, cs);
    }
  ret = INT2NUM (cret);
  return ret;
}

/*
 * Run the Ruby code in the passed-in string.  Return TRUE
 * if successful, or FALSE otherwise.
 */
static int
runruby (const char * line)
{
  int state;

  rb_eval_string_protect(line, &state);
  if (state)
    {
      /* Get the exception string and display it on the echo line.
       */
      VALUE exception = rb_errinfo ();
      if (RTEST(exception))
	{
	  VALUE msg = rb_funcall (exception, rb_intern("to_s"), 0);
	  eprintf ("ruby exception: %s", StringValueCStr (msg));
	}
      rb_set_errinfo (Qnil);		/* clear last exception */
      return FALSE;
    }
  return TRUE;
}

/*
 * Load the ruby library, initialize the pointers to the APIs,
 * define some C helper functions, and load the Ruby helper code
 * in pe.rb. Return TRUE on success, or FALSE on failure.
 */
static int
loadruby (void)
{
  int i, status, len;
  static char cmd[1024];
  char *path;

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
  rb_define_global_function("lineno", my_lineno, 0);
  rb_define_global_function("column", my_column, 0);
  rb_define_global_function("insert", my_insert, 1);
  rb_define_global_function("cbindtokey", my_bindtokey, 2);

  /* Construct the ruby statement:
   *  load '<PATH>/pe.rb'
   * where <PATH> is the directory name of the pe executable.
   */
   strcpy (cmd, "load '");
   len = strlen (cmd);
   path = cmd + len;

   /* Get the full path of the pe executable.  Replace pe with pe.rb,
    * the Ruby helper file, and load that file.
    */
   if ((len = readlink ("/proc/self/exe", path, sizeof (cmd) - len)) != -1)
   {
     char *p;

     /* Zap the final slash in the pathname.
      */
     path[len] = '\0';
     p = path + strlen (path);
     while (p >= path)
       {
	 --p;
	 if (*p == '/')
	   break;
       }
     *p = '\0';

     /* Terminate the command with the name of the Ruby helper file and
      * a matching single quote, then run the resulting command.
      */
     strcat (cmd, "/pe.rb'");
     return runruby (cmd);
   }

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

/*
 * Call a Ruby command function with the specified numeric argument n,
 * or nil if f is FALSE.
 */
int
rubycall (const char *name, int f, int n)
{
  char cmd[80];

  if (f == TRUE)
    snprintf(cmd, sizeof (cmd), "%s(%d)", name, n);
  else
    snprintf(cmd, sizeof (cmd), "%s(nil)", name);
  return runruby (cmd);
}

/*
 * Prompt for a string, and evaluate the string using the
 * Ruby interpreter.  Return TRUE if the string was evaluated
 * successfully, and FALSE if an exception occurred.
 */
int
rubystring (int f, int n, int k)
{
  int status;
  char line[NCOL];

  if ((status = loadruby ()) != TRUE)
    return status;
  if ((status = ereply ("Ruby code: ", line, sizeof (line))) != TRUE)
    return status;
  return runruby (line);
}

/*
 * Define a new MicroEMACS command that invokes a Ruby function.
 * The Ruby function takes a single parameter, which
 * is the numeric argument to the command, or nil
 * if there is no argument.
 */
int
rubycommand (int f, int n, int k)
{
  char line[NCOL];
  char *name;
  int status;

  if ((status = loadruby ()) != TRUE)
    return status;
  if ((status = ereply ("Ruby function: ", line, sizeof (line))) != TRUE)
    return status;
  if ((name = strdup (line)) == NULL)
    {
      eprintf ("Out of memory in rubycommand!");
      return FALSE;
    }

  /* Add a symbol with a null function pointer, which
   * indicates that this is a Ruby function.
   */
  if (symlookup (name) != NULL)
    {
      eprintf ("%s is already defined.", name);
      return FALSE;
    }
  else
    {
      keyadd (-1, NULL, name);
      return TRUE;
    }
}
