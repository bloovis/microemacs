# Replacment for built-in gcc-error command.

def gccerr(n)
  keepgoing = true
  while keepgoing
    l = $line
    if (l !~ /^In file included/ && l =~ /(.*):(\d+):(\d+): (.*)/)
      file = $1
      lno = $2
      col = $3
      err = $4
      if File.exist? file
	forw_line
	only_window
	split_window
	forw_window
	file_visit file
	goto_line lno.to_i
	forw_char col.to_i - 1
	echo "#{err}"
        return ETRUE
      else
	echo "File #{file} does not exist"
        return EFALSE
      end
    end
    keepgoing = forw_line == ETRUE
  end
  echo "No more gcc errors"
  return EFALSE
end

ruby_command "gccerr"
bind "gccerr", metactrl('e')
