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
  "rb_str_new_cstr",
  "rb_define_virtual_variable",
  "rb_string_value_ptr",
  "rb_load_protect",
  "ruby_init_loadpath",
  "rb_gv_get",
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
 * Helper function for Ruby bind (equivalent to
 * built-in command bind-to-key).  The first
 * parameter is a string containing the command
 * name, and the second parameter is the numeric
 * keycode.
 */
static VALUE
my_cbind (VALUE self, VALUE c, VALUE k)
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
 * Parameters:
 *   c = name of command
 *   f = argument flag (0 if no argument present, 1 if argument present)
 *   n = argument (integer)
 *   k = keystroke (only looked at by selfinsert)
 *   s = array of strings to be added to a queue of replies to eread
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
get_line (ID id)
{
  VALUE ret;
  VALUE utf8;

  ret = rb_str_new ((char *) lgets (curwp->w_dot.p), llength (curwp->w_dot.p));
  utf8 = rb_str_new_cstr("utf-8");
  rb_funcall (ret, rb_intern ("force_encoding"), 1, utf8);
  return ret;
}

/*
 * Replace the current line with the new string.
 */
static void
set_line (VALUE val, ID id)
{
  const char *str = StringValueCStr (val);
  int linelen = wllength (curwp->w_dot.p);
  curwp->w_dot.o = linelen;
  lreplace (linelen, str, TRUE);
}

/*
 * Get the character at the current position.  Return it
 * as a one-character UTF-8 string.
 */
static VALUE
get_char (VALUE self)
{
  VALUE ret;
  VALUE utf8;
  const uchar *s;
  int bytes;
  LINE *lp;

  lp = curwp->w_dot.p;
  s = wlgetcptr (lp, curwp->w_dot.o);

  /* If dot is at the end of the line, pretend that char is a newline.
   */
  if (s == lgets (lp) + llength (lp))
    {
      s = (const uchar *) "\n";
      bytes = 1;
    }
  else
    bytes = unblen (s, 1);
  ret = rb_str_new ((char *) s, bytes);
  utf8 = rb_str_new_cstr("utf-8");
  rb_funcall (ret, rb_intern ("force_encoding"), 1, utf8);
  return ret;
}

/*
 * Replace character at the current position with the new string.
 */
static void
set_char (VALUE val, ID id)
{
  const char *str = StringValueCStr (val);
  if (forwchar (TRUE, 1, KRANDOM) == TRUE)
    lreplace (1, str, TRUE);
  else
    /* We're at the end of the buffer.  Insert, don't replace.
     */
    linsert (strlen (str), 0, (char *) str);
}

/*
 * Get the current buffer's filename.
 */
static VALUE
get_filename (VALUE self)
{
  VALUE ret;

  ret = rb_str_new_cstr (curbp->b_fname);
  return ret;
}

/*
 * Set the current buffer's filename.
 */
static void
set_filename (VALUE val, ID id)
{
  const char *str = StringValueCStr (val);
  replyq_put (str);
  filename (FALSE, 0, KRANDOM);
}

/*
 * Get the current line number, 1 based so that it can
 * be used with goto-line, and for compatibility with
 * display-position.
 */
static VALUE
get_lineno (ID id)
{
  VALUE ret;

  ret = INT2NUM (lineno (curwp->w_dot.p) + 1);
  return ret;
}

/*
 * Set the current line number, 1 based so that it can
 * be used with goto-line.
 */
static void
set_lineno (VALUE val, ID id)
{
  int lineno = NUM2INT (val);
  gotoline (TRUE, lineno, KRANDOM);
}

/*
 * Get the current line length in UTF-8 characters, not bytes.
 */
static VALUE
my_linelen (VALUE self)
{
  VALUE ret;

  ret = INT2NUM (wllength (curwp->w_dot.p));
  return ret;
}

/*
 * Get the current offset into the current line.
 */
static VALUE
get_offset (ID id)
{
  VALUE ret;

  ret = INT2NUM (curwp->w_dot.o);
  return ret;
}

/*
 * Set the offset into the current line.
 */
static void
set_offset (VALUE val, ID id)
{
  int offset = NUM2INT (val);
  if (offset > wllength (curwp->w_dot.p))
    eprintf ("Offset too large");
  else
    {
      curwp->w_dot.o = offset;
      update ();
    }
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
      char *cs = StringValuePtr (s);
      int len = RSTRING_LEN (s);
      cret = insertwithnl (cs, len);
    }
  ret = INT2NUM (cret);
  return ret;
}

/*
 * Prompt the user and read back a reply, which is returned
 * as a string.  If the user aborts the reply with Control-G,
 * return nil.
 */
static VALUE
my_reply (VALUE self, VALUE s)
{
  VALUE ret;
  int cret;
  char buf[NCOL];

  if (!RB_TYPE_P(s, T_STRING))
    {
      eprintf ("reply parameter must be a string");
      ret = Qnil;
    }
  else
    {
      char *prompt = StringValueCStr (s);
      cret = ereply ("%s", buf, sizeof (buf), prompt);
      if (cret == ABORT)
	ret = Qnil;
      else
	ret = rb_str_new_cstr (buf);
    }
  return ret;
}

/*
 * Check if the last call to Ruby returned an exception.
 * If so, display the exception string on the echo line
 * and return FALSE.  Otherwise return TRUE.
 */
static int
check_exception (int state)
{
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
 * Load the specified Ruby script.  Return TRUE if the file was loaded
 * successfully.  If unsuccessful, display the exception information
 * on the echo line and return FALSE.
 */
static int
load_script (const char *path)
{
  VALUE script;
  int state;

  script = rb_str_new_cstr (path);
  rb_load_protect(script, 0, &state);
  return check_exception (state);
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
  return check_exception (state);
}

/*
 * Get the directory containing the pe executable.
 */
static char *
getexedir (void)
{
  int len;
  static char path[NFILEN];
  char *p;

  if ((len = readlink ("/proc/self/exe", path, sizeof (path) - 1)) != -1)
    {
      p = path + len;
      *p = '\0';
      while (p >= path)
        {
          --p;
          if (*p == '/')
            {
              *p = '\0';
              break;
            }
        }
    }
  else
    path[0] = '\0';
  return path;
}


/*
 * Load the Ruby library, initialize the pointers to the APIs,
 * define some C helper functions, and load the Ruby helper code
 * in pe.rb. Return TRUE on success, or FALSE on failure.
 */
static int
loadruby (void)
{
  int i, status;
  VALUE loadpath;
  VALUE dot;
  VALUE selfpath;
  static const char *libruby = STRINGIFY(LIBRUBY);

  if (ruby_handle != NULL)
    return TRUE;
  ruby_handle = dlopen(libruby, RTLD_LAZY);
  if (ruby_handle == NULL)
    {
      eprintf ("Unable to load %s", libruby);
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

  /* Initialize the load path for gems.
   */
  ruby_init_loadpath();

  /* Define global functions that can be called from Ruby.
   */
  rb_define_global_function("cmd", my_cmd, 5);
  rb_define_global_function("iscmd", my_iscmd, 1);
  rb_define_global_function("linelen", my_linelen, 0);
  rb_define_global_function("insert", my_insert, 1);
  rb_define_global_function("cbind", my_cbind, 2);
  rb_define_global_function("reply", my_reply, 1);

  /* Define some virtual global variables, along with
   * their getters and setters.
   */
  rb_define_virtual_variable ("$lineno", get_lineno, set_lineno);
  rb_define_virtual_variable ("$offset", get_offset, set_offset);
  rb_define_virtual_variable ("$line", get_line, set_line);
  rb_define_virtual_variable ("$char", get_char, set_char);
  rb_define_virtual_variable ("$filename", get_filename, set_filename);

  /* Add two directories to the load path:
   * - the current directory
   * - the directory containing the pe executable
   * This allows pe.rb, and possible other scripts as well, to be loaded
   * without specifying their full paths in the the most common cases.
   */
  loadpath = rb_gv_get("$LOAD_PATH");
  dot = rb_str_new_cstr (".");
  selfpath = rb_str_new_cstr (getexedir ());
  rb_funcall (loadpath, rb_intern ("push"), 2, dot, selfpath);

  /* Load the Ruby helper file, pe.rb.  It should be in either
   * the same directory as the pe executable, or in the current
   * directory.
   */
  return load_script ("pe.rb");
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
 * Prompt for a string containing the filename of a Ruby script,
 * and load the script.  Return TRUE if the file was loaded
 * successfully.  If unsuccessful, display the exception information
 * on the echo line and return FALSE.
 */
int
rubyload (int f, int n, int k)
{
  int status;
  char line[NCOL];

  if ((status = loadruby ()) != TRUE)
    return status;
  if ((status = ereply ("Ruby file to load: ", line, sizeof (line))) != TRUE)
    return status;
  return load_script (line);
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
