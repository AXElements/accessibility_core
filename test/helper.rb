require 'minitest/autorun'
require 'minitest/pride'

require 'accessibility/core'


if RUBY_ENGINE == 'ruby'
  def on_mri?
    true
  end
else
  def on_mri?
    false
  end
end

def on_macruby?
  !on_mri?
end


class MiniTest::Unit::TestCase

  def rand_nums count, range = 1_000
    Array.new count do
      rand range
    end
  end

  def rand_floats count, range = 1_000.0
    Array.new count do
      rand * range
    end
  end

end
