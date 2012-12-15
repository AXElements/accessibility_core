require 'accessibility/core/version'

if RUBY_ENGINE == 'macruby'

  ##
  # Whether or not we are running on MacRuby
  def on_macruby?
    true
  end

  require 'accessibility/core/macruby'

else

  ##
  # Whether or not we are running on MacRuby
  def on_macruby?
    false
  end

  require 'accessibility/core/mri'
end
