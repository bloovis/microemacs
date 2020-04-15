# dired.rb

require 'open3'

$nameoffset = 46

def showdir(dir)
  # Switch to the dired buffer and destroy any existing content
  use_buffer '*dired*'
  goto_bob
  set_mark
  goto_eob
  kill_region

  # Insert the contents of the directory, and move
  # the dot to the first entry other than . or ..
  fulldir = File.expand_path(dir)
  output, stderr_str, status = Open3.capture3('ls', '-laF', fulldir)
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
  return ETRUE
end

def dired(n)
  # Prompt for the directory name and expand it fully.
  dir = reply "Enter a directory name: "
  return showdir(dir)
end

def openfile(n)
  if $lineno < 3
    return EFALSE
  end
  filename = $line[$nameoffset..-2]
  old_lineno = $lineno
  old_offset = $offset
  goto_bob
  dir = $line[0..-3]
  $lineno = old_lineno
  $offset = old_offset
  full = dir + '/' + filename
  if full[-1] == '/'
    showdir(full)
  else
    file_visit(full)
  end
end
  
# Tell MicroEMACS about the new commands.

ruby_command "dired"
bind "dired", ctlx('d')
ruby_command "openfile"
bind "openfile", ctlxctrl('d')
