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
  modetable = { c: "c", rb: "ruby", cc: "cplusplus", cpp: "cplusplus", cr: "crystal", sh: "shell" }
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
  if mode.nil? && f =~ /\.(\w+)$/
    mode = modetable[$1.to_sym]
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
