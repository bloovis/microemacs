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

static void *ruby_handle;	/* Handle to libruby.so, NULL if rubyinit failed */
static int rubyinit_called;	/* TRUE if rubyinit has been called */
char rubyinit_error[100];	/* Buffer containing error message from rubyinit */

void *ruby_fptrs[100];

/*
 * Structure for passing arguments to rb_funcall dispatcher.
 */
struct callargs
{
  ID id;			/* ID of command function */
  VALUE parm;			/* numeric parameter to command */
};

/*
 * Do not change the first and last lines in the following table.  They
 * are used by makeapi.rb script to generate the trampoline code
 * for the Ruby APIs.
 */
const char *fnames[] =		/* Do not change this line */
{
  "ruby_setup",
  "ruby_cleanup",
  "rb_eval_string_protect",
  "rb_define_global_function",
  "rb_string_value_cstr",
  "rb_errinfo",
  "rb_set_errinfo",
  "rb_intern2",
  "rb_funcall",
  "rb_funcallv",
  "rb_str_new",
  "rb_str_new_static",
  "rb_str_new_cstr",
  "rb_define_virtual_variable",
  "rb_string_value_ptr",
  "rb_load_protect",
  "ruby_init_loadpath",
  "rb_gv_get",
  "rb_intern",
  "rb_mKernel",
  "rb_id2sym",
  "rb_cObject",
  "rb_protect",
  "rb_load",
  "rb_ruby_verbose_ptr",
#if __i386__
  "rb_num2long",
  "rb_int2big",
#else
  "rb_fix2int",
  "rb_num2int",
#endif
};		/* Do not change this line */

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
my_cbind (VALUE self, VALUE c, VALUE k, VALUE mode)
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
    {
      if (mode == Qtrue)
	setmodebinding (key, sp);
      else
	setbinding (key, sp);
    }

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
	    {
	      startsaveundo ();
	      cret = sp->s_funcp (flag, narg, key);
	      endsaveundo ();
	    }
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
get_line (ID id, VALUE *var)
{
  VALUE ret;
  VALUE utf8;
  LINE *lp;
  char *str;
  int len;

  /* Make a copy of the line with an extra space for a newline.
   */
  lp = curwp->w_dot.p;
  len = llength (lp);
  str = malloc (len + 1);
  if (str == NULL)
    {
      eprintf ("Out of memory in get_line!");
      return Qnil;
    }
  memcpy (str, lgets (lp), len);

  /* Append a newline if this is not the last line.
   */
  if (lp != lastline (curbp))
    {
      str[len] = '\n';
      ++len;
    }

  /* Make the copy of the line into a Ruby string, and free the copy.
   */
  ret = rb_str_new (str, len);
  utf8 = rb_str_new_cstr("utf-8");
  rb_funcall (ret, rb_intern ("force_encoding"), 1, utf8);
  free (str);
  return ret;
}

/*
 * Replace the current line with the new string;
 */
static void
set_line (VALUE val, ID id, VALUE *var)
{
  int len;
  char *str;

  /* Make a copy of the string, and zap any terminating
   * newline.
   */
  str = strdup (StringValueCStr (val));
  if (str == NULL)
    {
      eprintf ("Out of memory in set_line!");
      return;
    }
  len = strlen (str);
  if (len > 0 && str[len - 1] == '\n')
    str[len - 1] = '\0';

  /* Replace the line.
   */
  len = wllength (curwp->w_dot.p);
  curwp->w_dot.o = len;
  lreplace (len, str, TRUE);
}

/*
 * Get the character at the current position.  Return it
 * as a one-character UTF-8 string.
 */
static VALUE
get_char (VALUE self, VALUE *var)
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
set_char (VALUE val, ID id, VALUE *var)
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
get_filename (VALUE self, VALUE *var)
{
  VALUE ret;

  ret = rb_str_new_cstr (curbp->b_fname);
  return ret;
}

/*
 * Set the current buffer's filename.
 */
static void
set_filename (VALUE val, ID id, VALUE *var)
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
get_lineno (ID id, VALUE *var)
{
  VALUE ret;
  int l;

  l = lineno (curwp->w_dot.p) + 1;
  ret = INT2NUM (l);
  return ret;
}

/*
 * Set the current line number, 1 based so that it can
 * be used with goto-line.
 */
static void
set_lineno (VALUE val, ID id, VALUE *var)
{
  int lineno = NUM2INT (val);
  gotoline (TRUE, lineno, KRANDOM);
}

/*
 * Get the current buffer's flags.
 */
static VALUE
get_bflag (VALUE self, VALUE *var)
{
  VALUE ret;

  ret = INT2NUM (curbp->b_flag);
  return ret;
}

/*
 * Set the current buffer's flags.
 */
static void
set_bflag (VALUE val, ID id, VALUE *var)
{
  int flag = NUM2INT (val);
  curbp->b_flag = flag;
  curwp->w_flag |= WFMODE;
}

/*
 * Get the current buffer's name (not filename).
 */
static VALUE
get_bname (VALUE self, VALUE *var)
{
  VALUE ret;

  ret = rb_str_new_cstr (curbp->b_bname);
  return ret;
}

/*
 * Set the current buffer's name (not filename).
 */
static void
set_bname (VALUE val, ID id, VALUE *var)
{
  const char *str = StringValueCStr (val);
  if (strlen (str) >= NBUFN)
    {
      eprintf ("Buffer name too long!");
      return;
    }
  strcpy (curbp->b_bname, str);
  curwp->w_flag |= WFMODE;
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
get_offset (ID id, VALUE *var)
{
  VALUE ret;

  ret = INT2NUM (curwp->w_dot.o);
  return ret;
}

/*
 * Set the offset into the current line.
 */
static void
set_offset (VALUE val, ID id, VALUE *var)
{
  int offset = NUM2INT (val);
  if (offset > wllength (curwp->w_dot.p))
    eprintf ("Offset too large");
  else
    curwp->w_dot.o = offset;
}

/*
 * Get the current tab size.
 */
static VALUE
get_tabsize (VALUE self, VALUE *var)
{
  VALUE ret;

  ret = INT2NUM (tabsize);
  return ret;
}

/*
 * Set the current tab size.
 */
static void
set_tabsize (VALUE val, ID id, VALUE *var)
{
  settabsize (TRUE, NUM2INT (val), KRANDOM);
}

/*
 * Get the current fill column.
 */
static VALUE
get_fillcol (VALUE self, VALUE *var)
{
  VALUE ret;

  ret = INT2NUM (fillcol);
  return ret;
}

/*
 * Set the current fill column.
 */
static void
set_fillcol (VALUE val, ID id, VALUE *var)
{
  setfillcol (TRUE, NUM2INT (val), KRANDOM);
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
      startsaveundo ();
      int len = RSTRING_LEN (s);
      cret = insertwithnl (cs, len);
      endsaveundo ();
    }
  ret = INT2NUM (cret);
  return ret;
}

/*
 * Helper function for my_popup and check_exception that adds
 * an array of lines to the popup buffer.
 */
static int
add_lines (VALUE ary)
{
  int ci;
  VALUE len = rb_funcall (ary, rb_intern("length"), 0);
  int clen = FIX2INT (len);

  for (ci = 0; ci < clen; ci++)
    {
      VALUE i = LONG2FIX (ci);
      VALUE str = rb_funcall (ary, rb_intern ("slice"), 1, i);
      if (addline (StringValueCStr (str)) == FALSE)
	return FALSE;
    }
  return TRUE;
}

/*
 * Show the popup buffer with the specified string as the contents.
 */
static VALUE
my_popup (VALUE self, VALUE s)
{
  VALUE ret;
  int cret;

  if (!RB_TYPE_P(s, T_STRING))
    {
      eprintf ("popup parameter must be a string");
      cret = FALSE;
    }
  else
    {
      VALUE lines;

      /* Clear the popup buffer.
       */
      blistp->b_flag &= ~BFCHG;
      if (bclear (blistp) != TRUE)
	return FALSE;
      strcpy (blistp->b_fname, "");

      /* Split the string into lines and write each one to the popup buffer.
       */
      lines = rb_funcall (s, rb_intern("split"), 1, rb_str_new_cstr ("\n"));
      if (add_lines (lines) == FALSE)
	return FALSE;

      /* Display the popup buffer.
       */
      cret = popblist ();
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
 * Get an input character from the keyboard or profile.
 */
static VALUE
my_getkey (VALUE self)
{
  VALUE ret;

  ret = INT2NUM (getkey ());
  return ret;
}

/*
 * Initialize a mode for the current buffer.  The parameter
 * is the name of the mode.  The key binding table for the mode
 * is cleared, and can be filled in with subsequent calls to bind.
 * Return 1 if success, or zero otherwise.
 */
static VALUE
my_setmode (VALUE self, VALUE s)
{
  VALUE ret;
  int cret;

  if (!RB_TYPE_P(s, T_STRING))
    {
      eprintf ("setmode parameter must be a string");
      cret = FALSE;
    }
  else
    {
      char *cs = StringValuePtr (s);
      createmode (cs);
      cret = TRUE;
    }
  ret = INT2NUM (cret);
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
      /* Get the exception string and display it in the popup buffer.
       */
      VALUE exception = rb_errinfo ();
      if (RTEST(exception))
	{
	  VALUE msg;
	  VALUE msglines;
	  VALUE bt;

	  /* Clear the popup buffer.
	   */
	  blistp->b_flag &= ~BFCHG;
	  if (bclear (blistp) != TRUE)
	    return FALSE;
	  strcpy (blistp->b_fname, "");

	  /* Write the exception string to the popup buffer.
	   */
	  msg = rb_funcall (exception, rb_intern("to_s"), 0);
	  msglines = rb_funcall (msg, rb_intern("split"), 1, rb_str_new_cstr ("\n"));
	  if (add_lines (msglines) == FALSE)
	    return FALSE;

	  /* Write the backtrace strings to the popup buffer.
	   */
	  bt = rb_funcall (exception, rb_intern("backtrace"), 0);
	  if (add_lines (bt) == FALSE)
	    return FALSE;

	  /* Clear the exception and display the popup buffer.
	   */
	  rb_set_errinfo (Qnil);
	  return popblist ();
	}
      rb_set_errinfo (Qnil);		/* clear last exception */
      return FALSE;
    }
  return TRUE;
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
 * Helper function for loadscript.  This is a simple wrapper
 * for rb_load, required because rb_protect requires a function
 * that takes just one parameter, but rb_load takes two parameters.
 */
static VALUE
loadhelper (VALUE arg)
{
  rb_load (arg, 0);
  return Qnil;
}

/*
 * Load the specified Ruby script.  Return TRUE if the file was loaded
 * successfully.  If unsuccessful, display the exception information
 * on the echo line and return FALSE.
 *
 * Unfortunately, we can't use rb_load_protect here.  Despite its name,
 * it doesn't seem to protect against exceptions;  it aborts
 * with a segfault.  So instead, we call a wrapper for rb_load
 * from rb_protect, which allows us to catch any exceptions
 * in the loaded file.
 */
static int
loadscript (const char *path)
{
  VALUE script;
  int state;

  script = rb_str_new_cstr (path);
  rb_protect(loadhelper, script, &state);
  return check_exception (state);
}

/*
 * Helper function for rubyinit: store an error message in
 * rubyinit_error and return FALSE.
 */
static int
set_rubyinit_error (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vsnprintf (rubyinit_error, sizeof (rubyinit_error), fmt, ap);
  va_end (ap);
  return FALSE;
}


/*
 * Return error string from failed rubyinit call, or empty
 * string if rubyinit succeeded.
 */
const char *
rubyerror (void)
{
  return rubyinit_error;
}


/*
 * Load the Ruby library, initialize the pointers to the APIs,
 * define some C helper functions, and load the Ruby helper code
 * in pe.rb. Return TRUE on success, or FALSE on failure.
 * If quiet is TRUE, don't display an error message if the
 * Ruby shared library can't be loaded.
 */
int
rubyinit (int quiet)
{
  int i, status, namecount, fptrcount;
  VALUE loadpath;
  VALUE dir;
  static const char *libruby = STRINGIFY(LIBRUBY);
  static const char *global_pe_rb = STRINGIFY(PREFIX) "/share/pe/pe.rb";
  const char *home_pe_rb;
  static const char *local_pe_rb = "./.pe.rb";

  /* If we've been called before, return the status from that call.
   */
  if (rubyinit_called)
    return ruby_handle != NULL;
  rubyinit_called = TRUE;

  /* Make sure the API address table is big enough.
   */
  namecount = sizeof (fnames) / sizeof (fnames[0]);
  fptrcount = sizeof (ruby_fptrs) / sizeof (ruby_fptrs[0]);
  if (namecount >= fptrcount)
    {
      return set_rubyinit_error ("ruby_fptrs has %d entries but needs %d", fptrcount, namecount);
    }

  /* Open the Ruby runtime library.
   */
  ruby_handle = dlopen(libruby, RTLD_LAZY);
  if (ruby_handle == NULL)
    {
      return set_rubyinit_error ("Unable to load %s", libruby);
    }

  /* Query the addresses of our required Ruby API functions.
   */
  for (i = 0; i < namecount; i++)
    {
      ruby_fptrs[i] = dlsym (ruby_handle, fnames[i]);
      if (ruby_fptrs[i] == NULL)
	{
	  ruby_handle = NULL;
	  return set_rubyinit_error ("Unable to get address of ruby function %s", fnames[i]);
	}
      else
	{
	  /* printf("Address of %s is %p\n", fnames[i], ruby_fptrs[i]); */
	}
    }
  if ((status = ruby_setup ()) != 0)
    {
      ruby_handle = NULL;
      return set_rubyinit_error ("ruby_setup returned %d", status);
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
  rb_define_global_function("popup", my_popup, 1);
  rb_define_global_function("cbind", my_cbind, 3);
  rb_define_global_function("reply", my_reply, 1);
  rb_define_global_function("cgetkey", my_getkey, 0);
  rb_define_global_function("setmode", my_setmode, 1);

  /* Define some virtual global variables, along with
   * their getters and setters.
   */
  rb_define_virtual_variable ("$lineno", get_lineno, set_lineno);
  rb_define_virtual_variable ("$offset", get_offset, set_offset);
  rb_define_virtual_variable ("$line", get_line, set_line);
  rb_define_virtual_variable ("$char", get_char, set_char);
  rb_define_virtual_variable ("$filename", get_filename, set_filename);
  rb_define_virtual_variable ("$tabsize", get_tabsize, set_tabsize);
  rb_define_virtual_variable ("$fillcol", get_fillcol, set_fillcol);
  rb_define_virtual_variable ("$bflag", get_bflag, set_bflag);
  rb_define_virtual_variable ("$bname", get_bname, set_bname);

  /* Add the current directory and the location of pe.rb to the Ruby load path.
   * This allows the user to load other scripts without specifying
   * full paths.
   */
  loadpath = rb_gv_get("$LOAD_PATH");
  dir = rb_str_new_cstr (".");
  rb_funcall (loadpath, rb_intern ("push"), 1, dir);
  dir = rb_str_new_cstr (STRINGIFY(PREFIX) "/share/pe");
  rb_funcall (loadpath, rb_intern ("push"), 1, dir);

  /* Load the Ruby helper file, pe.rb.  It should be in PREFIX/share/pe.
   * Give an error if it doesn't exist.
   */
  if (access (global_pe_rb, R_OK) != F_OK)
    {
      ruby_handle = NULL;
      return set_rubyinit_error ("The file %s does not exist; cannot initialize Ruby",
				 global_pe_rb);
      return FALSE;
    }

  /* When loading pe.rb, temporarily suppress Ruby 2.7 warning about how
   * redefining Object#method_missing may cause infinite loop.
   */
  ruby_verbose = Qnil;
  if (loadscript (global_pe_rb) == FALSE)
    {
      ruby_handle = NULL;
      return FALSE;
    }
  ruby_verbose = Qtrue;

  /* Construct the name of $HOME/.pe.rb and load that file.
   * If it doesn't exist, try loading ./.pe.rb.  But don't
   * cause an error if either file doesn't exist, because
   * they are optional.
   */
  home_pe_rb = fftilde ("~/.pe.rb");
  if (access (home_pe_rb, R_OK) == F_OK)
    return loadscript (home_pe_rb);
  else if (access (local_pe_rb, R_OK) == F_OK)
    return loadscript (local_pe_rb);
  else
    return TRUE;
}

#if 0
static void
unloadrubylib (void)
{
  int status;

  /* destruct the VM */
  status = ruby_cleanup(0);
  if (status != 0)
    eprintf ("ruby_cleanup returned %d", status);
}
#endif

/*
 * Helper function for rubycall.  This performs the actual
 * call to the Ruby command.  It needs to have this function
 * signature because rb_protect requires it, and we want
 * to be able to catch any exceptions that occur during the call.
 */
static VALUE
dispatch (VALUE arg)
{
  VALUE ret;
  struct callargs *args = (struct callargs *) arg;

  ret = rb_funcall (Qnil, args->id, 1, args->parm);
  if (FIXNUM_P(ret))
    return ret;
  else
    return INT2NUM (FALSE);
}

/*
 * Call a Ruby command function with the specified numeric argument n,
 * or nil if f is FALSE.
 */
int
rubycall (const char *name, int f, int n)
{
  VALUE ret;
  struct callargs args;
  int state;

  /* First check if the function call will actually succeed.  Global
   * functions are actually private methods of Object, so we have
   * to set the second parameter to respond_to? to true so that
   * it will look for private methods.
   */
  args.id = rb_intern (name);
  ret = rb_funcall (rb_cObject, rb_intern("respond_to?"), 2, ID2SYM (args.id), Qtrue);
  if (ret == Qfalse)
    {
      eprintf ("%s is not a valid Ruby function", name);
      return FALSE;
    }

  /* Now we can go ahead and call the function inside a protected region.
   */
  if (f == TRUE)
    args.parm = INT2NUM (n);
  else
    args.parm = Qnil;
  ret = rb_protect (dispatch, (VALUE) &args, &state);
  if (check_exception (state) == FALSE)
    return FALSE;
  else
    {
      if (FIXNUM_P (ret))
	return NUM2INT (ret);
      else
	return FALSE;
    }
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

  if ((status = rubyinit (FALSE)) != TRUE)
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

  if ((status = rubyinit (FALSE)) != TRUE)
    return status;
  if ((status = ereply ("Ruby file to load: ", line, sizeof (line))) != TRUE)
    return status;
  return loadscript (line);
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

  if ((status = rubyinit (FALSE)) != TRUE)
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

/*
 * Call the Ruby function initmode to set up the
 * mode for the current buffer.
 */
void
rubymode (void)
{
  if (rubyinit (FALSE) != TRUE)
    return;
  rubycall ("initmode", FALSE, 0);
}
