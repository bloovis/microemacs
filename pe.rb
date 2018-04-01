# Class to encapsulate a MicroEMACS key code.

# Trap SIGINT so we can cleanly cause Ruby to
# exit instead of killing MicroEMACS.

Signal.trap('INT') do
   # puts "Got control-C"
   raise "Received SIGINT signal"
end

# Encapsulate a keystroke in its own type so that
# it can be distinguished from a numeric argument
# to cmd.

class Key

  CTRL = 0x10000000
  META = 0x20000000
  CTLX = 0x40000000
  KRANDOM = 0x80

  def initialize(c, f)
    @key = c.ord | f
  end

  def to_i
    @key
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
    if (@key & (CTLX | META | CTRL) != 0)
      s << (@key & 0x10ffff).chr.upcase
    else
      s << (@key & 0x10ffff).chr
    end
  end

end

def key(c)
  Key.new(c, 0)
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

# Invoke a MicroEMACS command:
#   c = name of command
#   f = argument flag (0 if no argument present, 1 if argument present)
#   n = argument (integer)
#   k = keystroke (only looked at by selfinsert)
#   s = array of strings to be fed to ereply

#def cmd (c, f, n, k, strings)
#  puts "cmd: c = #{c}, f = #{f}, n = #{n}, k = 0x#{k.to_s(16)}"
#  strings.each do |s|
#    puts "     string = #{s}"
#  end
#end

#def iscmd?(m)
#  [:forw_search, :self_insert, :replace_string, :forw_char].include? m
#end

# Check if an unknown method is MicroEMACS function.  If so,
# marshall its various arguments and call it; otherwise pass
# the exception on, which typically aborts the currently
# running Ruby program.

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
