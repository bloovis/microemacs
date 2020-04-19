# Skeletal mode hook functions.  Fill these in with
# key bindings as necessary and load them using
#   load "mode.rb"

def c_mode
  setmode "C"
  bind "gnu_indent", ctrl('j'), true
  echo "This is C mode"
end

def crystal_mode
  setmode "Crystal"
  bind "ruby_indent", ctrl('j'), true
  echo "This is Crystal mode"
end

def ruby_mode
  setmode "Ruby"
  bind "ruby_indent", ctrl('j'), true
  echo "This is Ruby mode"
end

def shell_mode
  setmode "Shell"
  echo "This is shell mode"
end

def cplusplus_mode
  setmode "C++"
  bind "gnu_indent", ctrl('j'), true
  echo "This is C++ mode"
end
