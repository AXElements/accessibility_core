require 'minitest/autorun'
require 'minitest/pride'

require 'accessibility/core'


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
