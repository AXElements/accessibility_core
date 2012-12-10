require 'test/helper'

class RangeTest < MiniTest::Unit::TestCase

  def test_to_range
    rng = 1..2
    assert_same rng, rng.to_range
  end

end
