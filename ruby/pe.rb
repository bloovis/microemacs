# This is the helper file for Ruby support in MicroEMACS.
# Copy it to /etc.

# MicroEMACS functions return a trinary value.  We have
# to use an "E" prefix to avoid conflicts with Ruby's
# builtin constants.

EFALSE = 0
ETRUE  = 1
EABORT = 2

# Trap SIGINT so we can cleanly cause Ruby to
# exit instead of killing MicroEMACS.

Signal.trap('INT') do
 raise "Received SIGINT signal"
end

# Encapsulate a keystroke in its own type so that
# it can be distinguished from a numeric argument
# to cmd.

class Key

  CTRL = 0x10000000
  META = 0x20000000
  CTLX = 0x40000000
  CHARMASK = 0x10ffff
  KRANDOM = 0x80

  def initialize(c, f=0)
    if c.class == String
      if f != 0
	@key = c.upcase.ord | f
      else
	@key = c.ord
      end
    else
      @key = c | f
    end
  end

  def to_i
    @key
  end

  def char
    (@key & CHARMASK).chr('UTF-8')
  end

  def ctrl?
    (@key & CTRL) != 0
  end

  def meta?
    (@key & META) != 0
  end

  def ctlx?
    (@key & CTLX) != 0
  end

  def normal?
    (@key & (CTRL | META | CTLX)) == 0
  end

  def ==(k)
    self.to_i == k.to_i
  end

  def to_s
    s = ""
    if (@key & CTLX) != 0
      s << "C-X "
    end
    if (@key & META) != 0
      s << "M-"
    end
    if (@key & CTRL) != 0
      s << "C-"
    end
    if self.normal?
      s << self.char
    else
      s << self.char.upcase
    end
  end

end

def key(c)
  Key.new(c)
end

def ctrl(c)
  Key.new(c, Key::CTRL)
end

def meta(c)
  Key.new(c, Key::META)
end

def ctlx(c)
  Key.new(c, Key::CTLX)
end

def metactrl(c)
  Key.new(c, Key::META | Key::CTRL)
end

def ctlxctrl(c)
  Key.new(c, Key::CTLX | Key::CTRL)
end

# Get a keystroke from the user, return it packaged in a Key object.

def getkey
  return Key.new(cgetkey, 0)
end

# Check if an unknown method is MicroEMACS function.  If so,
# marshall its various arguments and call it; otherwise pass
# the exception on, which typically aborts the currently
# running Ruby program.
#
# In order to invoke a MicroEMACS command, method_missing
# uses two C helper functions:
#
# iscmd(c) - returns true if the name c is a MicroEMACS command
# cmd(c, f, n, k, s) - invoke a MicroEMACS command:
#   c = name of command
#   f = argument flag (0 if no argument present, 1 if argument present)
#   n = argument (integer)
#   k = keystroke (only looked at by selfinsert)
#   s = array of strings to be fed to eread

def method_missing(m, *args, &block)
  # puts "method_missing #{m}"
  c = m.to_s
  super unless iscmd(c)
  # puts "Calling cmd #{m}"
  f = 0
  n = 1
  k = Key::KRANDOM
  s = []
  args.each do |arg|
    case arg
    when Fixnum
      # printf "arg = 0x%x\n", arg
      f = 1
      n = arg
    when Key
      # puts "key = #{arg.to_s}"
      k = arg.to_i
    when String
      s << arg
    else
      puts "unknown arg type: method = #{m}, arg = #{arg}, class = #{arg.class}"
    end
  end
  # puts "calling cmd(#{c}, #{f}, #{n}, #{k})"
  cmd(c, f, n, k, s)
end

# We have to define our own equivalent of bindtokey
# instead of using the command built into to MicroEMACS,
# because the latter prompts for a keystroke.
# Parameters:
#   name: string containing the function name
#   key:  keycode

def bind(name, key)
  cbind(name, key.to_i)	# Call C helper function
end

# Set the default encoding for strings.
Encoding.default_internal = 'UTF-8'
