require 'test/helper'
require 'accessibility/bridge'

class CGPointTest < MiniTest::Unit::TestCase

  def test_attributes
    p = CGPoint.new
    assert_respond_to p, :x
    assert_respond_to p, :y
  end

  def test_to_a
    p = CGPoint.new 1, 2
    assert_equal [1, 2], p.to_a
  end

  def test_to_point
    p = CGPoint.new
    assert_same p, p.to_point

    p = CGPoint.new 4, 2
    assert_same p, p.to_point
  end

  def test_initialize
    p = CGPoint.new
    assert_equal 0, p.x
    assert_equal 0, p.y

    x, y = rand_nums 2
    p = CGPoint.new x, y
    assert_equal x, p.x
    assert_equal y, p.y

    x, y = rand_floats 2
    p = CGPoint.new x, y
    assert_equal x, p.x
    assert_equal y, p.y
  end

  def test_inspect
    assert_match /Point x=1.0 y=2.0>/, CGPoint.new(1, 2).inspect
    assert_match /Point x=3.0 y=5.0>/, CGPoint.new(3, 5).inspect
  end

  if on_macruby?
    def test_to_ax
      value = CGPointZero.to_ax
      ptr   = Pointer.new CGPoint.type
      AXValueGetValue(value, 1, ptr)
      assert_equal CGPointZero, ptr.value, 'point makes a value'
    end
  end

end
