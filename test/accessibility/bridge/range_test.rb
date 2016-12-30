require 'test/helper'
require 'accessibility/bridge'

class RangeTest < Minitest::Test

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
