# This file doesn't define any new commands.  Instead,
# it demonstrates how the dot, mark, and the region between
# the two can be encapsulated in classes.

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

class Region
  def initialize
    @start = Dot.new
    @end = Mark.new

    # Make sure start comes before end
    if @start.lineno > @end.lineno ||
       (@start.lineno == @end.lineno &&
        @start.offset > @end.offset)
      tmp = @start
      @start = @end
      @end = tmp
    end
  end

  # Yield each line (or partial line) in the region
  def each
    old_lineno = $lineno
    old_offset = $offset
    lineno = @start.lineno 
    $lineno = lineno
    line = $line
    if @start.offset < line.length
      yield $line[@start.offset..-1]
    end
    lineno += 1
    while lineno < @end.lineno
      $lineno = lineno
      yield $line
      lineno += 1
    end
    if @end.offset > 0
      $lineno = lineno
      yield $line[0..@end.offset - 1]
    end
    $lineno = old_lineno
    $offset = old_offset
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

def testregion
  region = Region.new
  region.each do |line|
    if reply("line = '#{line}'.  Hit enter to continue: ") == nil
      break
    end
  end
end
