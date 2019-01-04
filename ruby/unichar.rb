# unichar.rb
#
# MicroEMACS function for inserting accented European characters.
# Examine the character under the cursor, which should be a normal
# ASCII letter, and offer a set of accented replacements for it.

def unichar(n)
  replacements = {
    'a' => 'äàáâãåæ',
    'c' => 'ç',
    'e' => 'èéêë',
    'i' => 'ìíîï',
    'n' => 'ñ',
    'o' => 'òóôõö',
    'u' => 'úûü',
    'y' => 'ýÿ',
    'A' => 'ÀÁÂÃÄÅÆ',
    'C' => 'Ç',
    'E' => 'ÈÉÊË',
    'I' => 'ÌÍÎÏ',
    'N' => 'Ñ',
    'O' => 'ÒÓÔÕÖØ',
    'U' => 'ÙÚÛÜ',
    'Y' => 'Ý'
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
