# gems
require 'accessibility/bridge'

# internal deps
require 'accessibility/core/version'

if on_macruby?
  require 'accessibility/core/macruby'
else
  require 'accessibility/core/core.bundle'
end

