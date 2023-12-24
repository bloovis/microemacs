# ledger.rb
#
# MicroEMACS functions for use with the ledger accounting program
# (https://www.ledger-cli.org/).  To load this automatically when editing
# transactions, copy this file to your ledger directory and rename it
# to .pe.rb, then change the setting of $acct at the end of the file
# to the name of a relevant bank account.

# We assume that aliases are defined in a file called aliases.dat
# that is included in ledger.dat.

require 'open3'

def readaliases
  $aliases = {}
  $accounts = {}
  File.open('aliases.dat') do |file|
    file.each do |line|
      if line =~ /^alias\s+(\w+)\s+=\s+(\w.*)/
        al = $1
        ac = $2
        $aliases[ac] = al
	$accounts[al] = ac
      end
    end
  end
end

# Examine the current line for a date and partial account name,
# and replace it with a copy of the most recent transaction for
# that account, but using the newly entered date.

def xact(n)
  line = $line
  lineno = $lineno
  if line =~ /^(\d\d\d\d\/\d\d\/\d\d)\s+(\(\d+\)\s+)?(\w+.*)/
    date = $1
    checkno = $2
    payee = $3.gsub(/'/, "\\'")
    # echo "payee = #{payee}"
    transaction, stderr_str, status = Open3.capture3('ledger', '--no-aliases',
      '-f', 'ledger.dat', 'xact', date, payee)
    if status.to_i != 0
      popup stderr_str
      return EFALSE
    end
    if transaction.length == 0
      echo "No matching transaction"
      return EFALSE
    end
    lines = transaction.split("\n")
    $line = ''
    lines.each do |line|
      if line =~ /^(\s+)([a-zA-Z].*[a-zA-Z])(\s\s.*)/ ||
	 line =~ /^(\s+)([a-zA-Z].*[a-zA-Z])(.*)/ ||
	 line =~ /^(\s+\()([a-zA-Z].*[a-zA-Z])(\).*)/
	leader = $1
	acct = $2
	trailer = $3
	# Trim off one space from the leader.
	leader.sub!(/^\s/, '')
	# Replace leading spaces (after optional closing paren)
	# in the trailer with two tabs.
	if trailer
	  trailer.sub!(/^(\)?)\s+/, "\\1\t\t")
	end
	line = leader + acct + trailer
      end
      insert line + "\n"
    end
    $lineno = lineno
    $offset = 11
    insert checkno if checkno
    return ETRUE
  else
    echo "Transaction must start with date and partial payee name"
    return EFALSE
  end
end

# Prompt for a date, and return it if it is valid, or nil otherwise.
# The date can be in three formats: YYYY/MM/DD, D/M/20YY, or M/D.  If the latter
# two are entered, convert them to YYYY/MM/DD.

def getdate
  line = reply "Enter a date: "
  if line.nil?
    return nil
  end
  if line =~ /^(\d\d\d\d\/\d\d\/\d\d)$/
    return $1
  elsif line =~ /^(\d+)\/(\d+)\/(20\d\d)$/
    return sprintf("%04d/%02d/%02d", $3, $1, $2)
  elsif line =~ /^(\d+)\/(\d+)$/
    return Time.now.strftime('%Y') + sprintf('/%02d/%02d', $1, $2)
  else
    echo "Date must be in format YYYY/MM/DD"
    return nil
  end
end

# Insert the most recently set date.  If an argument is present,
# prompt for a new date to be used in subsequent invocations.
# The initial date is today's date.

def insdate(n)
  if n
    d = getdate
    if d.nil?
      return EFALSE
    end
    $date = d
  end
  insert $date + ' '
  return ETRUE
end

# Return the word under the cursor and its offset in the line, and move
# the cursor past the end of the word.  If there is no word under the cursor
# return an empty string.

def getword
  line = $line
  len = line.length
  offset = $offset
  while offset > 0 && line[offset - 1] =~ /\w/
    offset -= 1
  end
  start = offset
  while offset < len && line[offset] =~ /\w/
    offset += 1
  end
  return line[start..offset - 1], start
end

# Replace the string al with newal at the specified offset in the current line.

def replace(al, newal, offset)
  if al == newal
    echo "[#{al} unchanged]"
    return ETRUE
  end
  line = $line
  $line = line[0..offset - 1] + newal + line[offset + al.length..-1]
  echo "[#{al} replaced with #{newal}]"
  return ETRUE
end

# Prompt for a partial alias name.  Insert the name if it is unique and found
# in the aliases table.  If multiple aliases start with the same string,
# prompt for which one to insert.  If none is found, say so and don't insert
# anything.

def insalias(n)
  al, offset = getword
  if al.length == 0
    echo "No word under cursor"
    return EFALSE
  end
  matches = []
  $accounts.each_key do |key|
    if key =~ /#{al}/
      matches << key
    end
    if matches.length == 10
      break
    end
  end
  if matches.length == 0
    echo "No alias found containing #{al}"
    return EFALSE
  end
  if matches.length == 1
    return replace al, matches[0], offset
  end
  prompt = ''
  n = 0
  matches.each do |match|
    if prompt.length > 0
      prompt << ','
    end
    prompt << "#{n}=#{match.gsub(/#{al}/, '*')}"
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
      ch = k.char
      if ch >= '0' && ch <= '9'
	n = ch.ord - '0'.ord
	if n < matches.length
	  return replace al, matches[n], offset
	end
      end
    end
  end
end

# Mark the current transaction as cleared (if not already cleared), save the file,
# and display the cleared balance for a predetermined account.  If there is an
# argument present, prompt for the name of the account and use it for subsequent
# invocations.

def cleared(n)
  if n
    $acct = reply "Enter account name: "
  end
  line = $line
  if line =~ /^(\d\d\d\d\/\d\d\/\d\d\s+)(.*)/ ||
     line =~ /^(\s+)(\w.*)/
    leader = $1
    rest = $2
    if rest !~ /^\*/
      $line = leader + '* ' + rest
    end
  end
  file_save
  line = `ledger -f ledger.dat bal --cleared '#{$acct}'`
  if line =~ /\$([\-\d\.,]+)/
    echo "#{$acct} cleared balance = #{$1}"
    return ETRUE
  else
    echo "invalid balance line: #{line.strip}"
    return EFALSE
  end
end

# Prompt for a date, then find the transaction whose date is greater than
# or equal to that date, and insert a new line with that date.

def finddate(n)
  d = getdate
  if d.nil?
    return EFALSE
  end
  $date = d
  goto_bob
  keepgoing = true
  while keepgoing
    line = $line
    if line =~ /^(\d\d\d\d\/\d\d\/\d\d)/
      if $1 >= $date
	break
      end
    end
    keepgoing = forw_line == ETRUE
  end
  goto_bol
  ins_nl_and_backup
  insert $date + ' '
  return ETRUE
end

# Tell MicroEMACS about the new commands.

ruby_command "xact"
bind "xact", ctlx('x')
ruby_command "insdate"
bind "insdate", ctlx('d')
ruby_command "cleared"
bind "cleared", ctlx('c')
ruby_command "insalias"
bind "insalias", ctlx('a')
ruby_command "finddate"
bind "finddate", ctlx('f')

# Set up some global variables used by the commands.

readaliases
$date = Time.now.strftime("%Y/%m/%d")	# Used by the insdate command
$acct = 'Assets:TBTF Bank'		# Used by the cleared command
