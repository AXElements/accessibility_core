require 'accessibility/bridge'

if on_macruby?
  framework 'Cocoa'
else
  require 'accessibility/extras.bundle'
end

