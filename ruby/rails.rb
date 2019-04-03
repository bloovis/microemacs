# Replacements for the old (now deleted) built-in Rails-related commands.
# These are a little smarter, though; they attempt
# to figure out class and method names by examining
# the source code in the current window.

require 'rubygems'
require 'active_support/inflector'

# Walk up the directory tree looking for the root directory
# of the Rails project.  Return that directory name if found,
# or nil if not found within five directories up.

def findroot
  tries = 0
  root = "."
  while tries < 5
    dir = root + '/app/views'
    if File.directory?(dir)
      return root
    end
    tries += 1
    if root == '.'
      root = '..'
    else
      root = root + '/..'
    end
  end
  echo "Cannot find Rails root directory"
  return nil
end

# Scan backwards looking for the start of method, and if found,
# return its name; otherwise, return nil.

def findmethod
  name = nil
  old_lineno = $lineno
  old_offset = $offset
  while true
    if $line =~ /\s*def\s+(\w+)/
      name = $1
      break
    elsif back_line != ETRUE
      break
    end
  end
  $lineno = old_lineno
  $offset = old_offset
  return name
end

# Scan backwards for the start of a class, and if found,
# return the class name; otherwise, return nil.

def findclass
  name = nil
  old_lineno = $lineno
  old_offset = $offset
  while true
    if $line =~ /\s*class\s+(\w+)/
      name = $1
      break
    elsif back_line != ETRUE
      break
    end
  end
  $lineno = old_lineno
  $offset = old_offset
  return name
end

# Load the file containing the view for the current class
# and method.

def loadview(n)
  klass = n.nil? ? findclass : nil
  if klass.nil?
    klass = reply "Class name: "
    if klass.nil? || klass.length == 0
      return EFALSE
    end
  end
  if klass =~/(\w+)Controller/
    klass = $1
  end
  func = n.nil? ? findmethod : nil
  if func.nil?
    func = reply "Method name: "
    if func.nil? | func.length == 0
      return EFALSE
    end
  end
  file = "#{$railsroot}/app/views/#{klass.pluralize.underscore}/#{func}.html.erb"
  if File.exist?(file)
    return file_visit file
  else
    echo "No such file: #{file}"
    return EFALSE
  end
end

# Load the file containing the model for the current class.

def loadmodel(n)
  klass = n.nil? ? findclass : nil
  if klass.nil?
    klass = reply "Class name: "
    if klass.nil? || klass.length == 0
      return EFALSE
    end
  end
  if klass =~/(\w+)Controller/
    klass = $1
  end
  file = "#{$railsroot}/app/models/#{klass.singularize.underscore}.rb"
  if File.exist?(file)
    return file_visit file
  else
    echo "No such file: #{file}"
    return EFALSE
  end
end

# Load the file containing the controller for the current class.

def loadcontroller(n)
  klass = n.nil? ? findclass : nil
  if klass.nil?
    klass = reply "Class name: "
    if klass.nil? || klass.length == 0
      return EFALSE
    end
  end
  if klass =~/(\w+)Controller/
    klass = $1
  end
  file = "#{$railsroot}/app/controllers/#{klass.pluralize.underscore}_controller.rb"
  if File.exist?(file)
    return file_visit file
  else
    echo "No such file: #{file}"
    return EFALSE
  end
end

$railsroot = findroot

ruby_command "loadview"
ruby_command "loadmodel"
ruby_command "loadcontroller"

bind "loadview", ctlx('v')
bind "loadmodel", ctlx('m')
bind "loadcontroller", ctlx('c')
