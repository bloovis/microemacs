# This file doesn't define any new commands.  Instead,
# it demonstrates how the dot and mark can be encapsulated
# in tiny Dot and Mark classes, respectively.

class Pos
  def initialize(ln, off)
    @ln = ln
    @off = off
  end
  def lineno
    @ln
  end
  def offset
    @off
  end
end

class Dot < Pos
  def initialize
    super $lineno, $offset
  end
end

class Mark < Pos
  def initialize
    if swap_dot_and_mark == EFALSE
      super 0, 0
    else
      super $lineno, $offset
      swap_dot_and_mark
    end
  end
end

def testdot
  dot = Dot.new
  echo "Dot at line #{dot.lineno}, offset #{dot.offset}"
end

def testmark
  mark = Mark.new
  if mark.lineno == 0
    echo "No mark has been set"
  else
    echo "Mark at line #{mark.lineno}, offset #{mark.offset}"
  end
end
