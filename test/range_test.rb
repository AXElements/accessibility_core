require 'test/helper'

if on_macruby?
class RangeTest < MiniTest::Unit::TestCase

  def test_to_ax
    assert_equal CFRange.new(5, 4).to_ax, (5..8).to_ax
  end

  def test_to_ax_raises_when_given_negative_indicies
    assert_raises(ArgumentError) { (1..-10).to_ax  }
    assert_raises(ArgumentError) { (-5...10).to_ax }
  end

end
end
