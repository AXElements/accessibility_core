require 'accessibility/bridge'

if on_macruby?
  framework 'Cocoa'
else
  require 'accessibility/running_application.bundle'
end

