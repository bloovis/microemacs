# unichar.rb
#
# MicroEMACS function for inserting accented European characters.
# Examine the character under the cursor, which should be a normal
# ASCII letter, and offer a set of accented replacements for it.

def unichar(n)
  replacements = {
    'a' => 'äàáâãåæāăą',
    'c' => 'çćĉċč',
    'd' => 'ďđ',
    'e' => 'èéêë',
    'g' => 'ĝğġģ',
    'h' => 'ĥħ',
    'i' => 'ìíîïĩīĭįıĳ',
    'j' => 'ĵ',
    'k' => 'ķĸ',
    'l' => 'ĺļľŀł',
    'n' => 'ñńņňŉŋ',
    'o' => 'òóôõöōŏőœ',
    'r' => 'ŕŗř',
    's' => 'śŝşš',
    't' => 'ţťŧ',
    'u' => 'úûüũūŭůűų',
    'w' => 'ŵ',
    'y' => 'ýÿŷ',
    'z' => 'źżž',
    'A' => 'ÀÁÂÃÄÅÆĀĂĄ',
    'C' => 'ÇĆĈĊČ',
    'D' => 'ĎĐ',
    'E' => 'ÈÉÊËĒĔĖĘĚ',
    'G' => 'ĜĞĠĢ',
    'H' => 'ĤĦ',
    'I' => 'ÌÍÎÏĪĬĮİĲ',
    'J' => 'Ĵ',
    'K' => 'Ķ',
    'L' => 'ĻĽĿŁ',
    'N' => 'ÑŃŅŇŊ',
    'O' => 'ÒÓÔÕÖØŌŎŐŒ',
    'R' => 'ŔŖŘ',
    'S' => 'ŜŞŠ',
    'T' => 'ŢŤŦ',
    'U' => 'ÙÚÛÜŨŪŬŮŰŲ',
    'W' => 'Ŵ',
    'Y' => 'ÝŶŸ',
    'Z' => 'ŹŻŽ'
  }

  ch = $char
  repls = replacements[ch]
  if repls.nil?
    echo "Unrecognized character '#{ch}'"
    return EFALSE
  end
  prompt = ''
  n = 0
  repls.each_char do |r|
    if prompt.length > 0
      prompt << ','
    end
    prompt << "#{n}=#{r}"
    n += 1
  end
  prompt << ": "
  while true
    echo prompt
    k = getkey
    if k == ctrl('g')
      echo ''
      return EFALSE
    end
    if k.normal?
      kch = k.char
      if kch >= '0' && kch <= '9'
	n = kch.ord - '0'.ord
	if n < repls.length
	  newch = repls[n]
	  $char = newch
	  echo "[#{ch} replaced with #{newch}]"
	  return ETRUE
	end
      end
    end
  end
end

# Tell MicroEMACS about the new command.

ruby_command "unichar"
bind "unichar", metactrl('c')
