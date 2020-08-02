# dired.rb

require 'open3'

$nameoffset = 46

def viewfile(filename, kind)
  case kind
  when :visit
    file_visit filename
  when :open
    only_window
    split_window
    forw_window
    file_visit filename
  when :display
    only_window
    split_window
    forw_window
    file_visit filename
    back_window
  end
end

def showdir(dir)
  # Switch to the dired buffer and destroy any existing content.
  use_buffer '*dired*'
  $bflag = 0
  goto_bob
  set_mark
  goto_eob
  kill_region

  # Insert the contents of the directory, and move
  # the dot to the first entry other than . or ..
  fulldir = File.expand_path(dir)
  output, stderr_str, status = Open3.capture3('ls', '-laF', fulldir)
  stderr_str += ""	# suppress warning about unused variable
  insert fulldir + ":\n"
  insert output

  # Figure out the offset of the filename in each line.
  $lineno = 3
  l = $line
  if l =~ /^(.*)\.\//
    $nameoffset = $1.length
  else
    $nameoffset = 46	# just a guess
  end
  $lineno = 5
  $offset = $nameoffset

  # Make the buffer readonly, attach a mode to it, and bind
  # some keys to special functions.
  $bflag = BFRO
  setmode "dired"
  bind "visitfile", ctrl('m'), true
  bind "openfile", key('o'), true
  bind "displayfile", ctrl('o'), true
  return ETRUE
end

def handlefile(kind)
  line = $line.chomp
  if $lineno == 1
    # Extract the portion of the path up to next / after the cursor on line 1
    line.gsub!(/:/, '')
    offset = $offset
    after = line[offset..-1]
    if after =~ /^([^\/]*)\//
      afterlen = $1.length
      filename = line[1..offset+afterlen]
    else
      filename = line[1..-1]
    end
    dir = ''
  else
    # Extract the filename from the directory listing.
    if $lineno < 3
      return EFALSE
    end
    old_lineno = $lineno
    old_offset = $offset
    goto_bob
    dir = $line[0..-3]
    $lineno = old_lineno
    $offset = old_offset
    filename = line[$nameoffset..-1]
    filename.gsub!(/\*/, '')		# executable file
    filename.gsub!(/ -> .*/, '')	# symbolic link
  end
  full = dir + '/' + filename
  if File.directory?(full)
    if full[-1] != '/'
      full << '/'
    end
    showdir(full)
  else
    viewfile(full, kind)
  end
end
  
def visitfile(n)
  handlefile :visit
end

def openfile(n)
  handlefile :open
end

def displayfile(n)
  handlefile :display
end

def dired(n)
  # Prompt for the directory name and expand it fully.
  dir = reply "Enter a directory name: "
  unless dir
    return EFALSE
  end
  return showdir(dir)
end

# Tell MicroEMACS about the new commands.

ruby_command "dired"
bind "dired", ctlx('d')
ruby_command "visitfile"
ruby_command "openfile"
ruby_command "displayfile"
