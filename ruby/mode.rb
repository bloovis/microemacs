# mode.rb - Ruby portion of the implementation of mode feature
# similar to major modes in Emacs
#
# A mode is a record containing a name and a set of key bindings;
# this record is attached to a particular buffer.  Key bindings
# in a mode override those in the global key binding table.
#
# MicroEMACS calls initmode whenever a new buffer is opened.
# It uses the following two stratagies to determine the name
# of the mode for the buffer:
#
#   1. Look at the first non-blank line, and if it contains
#      the string -*-MODE-*-, MODE specifies the mode name
#   2. If strategy #1 fails, look at the filename, and see if
#      it matches any of the patterns in the $modetable below
#
# If a mode name is determined, run the function MODE_mode,
# if present, where MODE is the mode name.  The user must provide
# one or more of these functions to implement the desired modes.
#
# A mode function will typically perform any necessary buffer
# manipulation, then define one or more MicroEMACS functions,
# and finally bind some keys to that mode.  See dired.rb
# for an example.

# Table associating filename patterns to mode names.
$modetable = {
  /\.c$/ => "c",
  /\.rb$/ => "ruby",
  /\.cc$/ => "cplusplus",
  /\.cpp$/ => "cplusplus",
  /\.cr$/ => "crystal",
  /\.sh$/ => "shell"
}

def c_mode
  echo "This is C mode"
end

def crystal_mode
  echo "This is Crystal mode"
end

def ruby_mode
  echo "This is Ruby mode"
end

def shell_mode
  echo "This is shell mode"
end

def cplusplus_mode
  echo "This is C++ mode"
end

def initmode(n)
  mode = nil
  f = $filename

  # Try to determine the mode from the first non-blank line
  # in the file, which could contain a mode specification like this:
  #   -*-Mode-*-
  # The mode is lower-cased before checking for the hook function,
  goto_bob
  keepgoing = true
  while keepgoing
    l = $line.chomp
    if l =~ /^\s*$/	# skip leading blank lines
      keepgoing = forw_line == ETRUE
    else
      keepgoing = false
      if l =~ /-\*-(\w+)-\*-/
	mode = $1.downcase
      end
    end
  end
  goto_bob
   
  # Try to determine the mode from the file extension.
  if mode.nil?
    $modetable.each do |r, m|
      if f =~ r
	mode = m
	break
      end
    end
  end

  # If a mode hook function exists, call it
  if mode.nil?
    echo "[unable to determine mode]"
    return
  end
  hook = mode + "_mode"
  if Object.respond_to?(hook, true)
    Object.send hook
  else
    echo "[unknown mode hook #{hook}]"
  end
end
