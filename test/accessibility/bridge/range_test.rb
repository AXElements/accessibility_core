require 'test/helper'
require 'accessibility/bridge'

class RangeTest < MiniTest::Unit::TestCase

  if on_macruby?
    def test_to_ax
      assert_equal CFRange.new(5, 4).to_ax, (5..8).to_ax
    end

    def test_to_ax_raises_when_given_negative_indicies
      assert_raises(ArgumentError) { (1..-10).to_ax  }
      assert_raises(ArgumentError) { (-5...10).to_ax }
    end
  end

  def test_relative_to
    assert_equal 0..10, ( 0 ..  10).relative_to(11)
    assert_equal 1..9,  ( 1 ..  9 ).relative_to(11)
    assert_equal 0..10, ( 0 ..  15).relative_to(11)
    assert_equal 0..10, ( 0 ..  11).relative_to(11)

    assert_equal 0..10, ( 0 .. -1 ).relative_to(11)
    assert_equal 0..9,  ( 0 .. -2 ).relative_to(11)
    assert_equal 1..9,  ( 1 .. -2 ).relative_to(11)

    assert_equal 0..10, (-11..  10).relative_to(11)
    assert_equal 6..10, (-5 ..  10).relative_to(11)
    assert_equal 1..6,  (-10.. -5 ).relative_to(11)

    assert_equal 4..10, ( 4 ... 11).relative_to(11)
    assert_equal 4..10, ( 4 ... 15).relative_to(11)
    assert_equal 4..9,  ( 4 ... 10).relative_to(11)
    assert_equal 4..9,  ( 4 ...-1 ).relative_to(11)
    assert_equal 0..9,  (-11...-1 ).relative_to(11)
  end

end
