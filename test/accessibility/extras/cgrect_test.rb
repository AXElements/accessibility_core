require 'test/helper'
require 'accessibility/extras'

class CGRectTest < Minitest::Test

  def test_flip!
    size = NSScreen.mainScreen.frame.size
    assert_equal [0,       size.height, 0,     0].to_rect, CGRect.new.flip!
    assert_equal [100, size.height-200, 100, 100].to_rect, [100,100,100,100].to_rect.flip!
  end

  def test_flip_twice_returns_to_original_rect
    assert_equal CGRect.new, CGRect.new.flip!.flip!
  end

end
