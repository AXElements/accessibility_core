require 'test/helper'

class CGRectTest < MiniTest::Unit::TestCase

  def test_attributes
    r = CGRect.new
    assert_respond_to r, :origin
    assert_respond_to r, :size
  end

  def test_to_a # we want to match MacRuby here
    r = CGRect.new
    assert_equal [CGPoint.new, CGSize.new], r.to_a
  end

  def test_to_rect
    r = CGRect.new
    assert_same r, r.to_rect

    r = CGRect.new CGPoint.new(4, 2), CGSize.new(8, 3)
    assert_same r, r.to_rect
  end

  def test_initialize
    r = CGRect.new
    assert_equal 0, r.origin.x
    assert_equal 0, r.origin.y
    assert_equal 0, r.size.width
    assert_equal 0, r.size.height

    x, y, w, h = rand_nums 4
    p = CGPoint.new x, y
    s = CGSize.new w, h
    r = CGRect.new p, s
    assert_equal p, r.origin
    assert_equal s, r.size
  end

  def test_inspect
    p1 = CGPoint.new *rand_nums(2)
    p2 = CGPoint.new *rand_nums(2)
    s1 = CGSize.new *rand_nums(2)
    s2 = CGSize.new *rand_nums(2)
    r1 = CGRect.new p1, s1
    r2 = CGRect.new p2, s2
    assert_match /Rect origin=#{p1.inspect} size=#{s1.inspect}>/, r1.inspect
    assert_match /Rect origin=#{p2.inspect} size=#{s2.inspect}>/, r2.inspect
  end

  if on_macruby?
    def test_to_ax
      value = CGRectZero.to_ax
      ptr   = Pointer.new CGRect.type
      AXValueGetValue(value, 3, ptr)
      assert_equal CGRectZero, ptr.value, 'rect makes a value'
    end
  end

  def test_flip!
    size = NSScreen.mainScreen.frame.size
    assert_equal [0,       size.height, 0,     0].to_rect, CGRect.new.flip!
    assert_equal [100, size.height-200, 100, 100].to_rect, [100,100,100,100].to_rect.flip!
  end

  def test_flip_twice_returns_to_original_rect
    assert_equal CGRect.new, CGRect.new.flip!.flip!
  end

end
