require 'accessibility/bridge'

if on_macruby?
  require 'accessibility/highlighter/macruby'
else
  require 'accessibility/highlighter/highlighter.bundle'
end

