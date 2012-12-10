require 'test/helper'

class ArrayTest < MiniTest::Unit::TestCase

  def test_to_point
    x, y = rand_nums 2
    p = [x, y].to_point
    assert_kind_of CGPoint, p
    assert_equal x, p.x
    assert_equal y, p.y
  end

  def test_to_size
    w, h = rand_nums 2
    s = [w, h].to_size
    assert_kind_of CGSize, s
    assert_equal w, s.width
    assert_equal h, s.height
  end

  def test_to_rect
    x, y, w, h = rand_nums 4
    r = [x, y, w, h].to_rect
    assert_kind_of CGRect, r
    assert_equal x, r.origin.x
    assert_equal y, r.origin.y
    assert_equal w, r.size.width
    assert_equal h, r.size.height
  end

end
