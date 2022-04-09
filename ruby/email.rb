# Produce a popup window showing email addresses from notmuch
# that match the string under the cursor.

require 'open3'

# Return the email address under the cursor, or any empty string
# if there doesn't appear to be an email address there.

def getemail
  line = $line
  len = line.length
  offset = $offset
  # Find the beginning of the email address.
  while offset > 0 && line[offset - 1] =~ /[\w@\.-]/
    offset -= 1
  end
  start = offset
  # Find the end of the email address.
  while offset < len && line[offset] =~ /[\w@\.-]/
    offset += 1
  end
  return line[start..offset - 1]
end

# If there is an email address under the cursor, pop a window showing
# the possible matches for that email address from notmuch.

def findemails(n)
  email = getemail
  if email.length == 0
    echo "No email address found under cursor"
    return EFALSE
  end
  addresses, stderr_str, status = Open3.capture3('notmuch', 'address', "from:#{email}")
  if status.to_i != 0
    popup stderr_str
    return EFALSE
  end
  popup addresses
  return ETRUE
end

ruby_command "findemails"
bind "findemails", ctlx('m')
