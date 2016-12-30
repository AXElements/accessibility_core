require 'test/helper'
require 'accessibility/extras'

class NSScreenTest < Minitest::Test

  def test_main_screen
    assert_respond_to NSScreen, :mainScreen
    assert_kind_of NSScreen, NSScreen.mainScreen
  end

  def test_screens
    assert_respond_to NSScreen, :screens
    assert_kind_of Array, NSScreen.screens
    NSScreen.screens.each do |screen|
      assert_kind_of NSScreen, screen
    end
  end

  def test_screen_frame
    assert_respond_to NSScreen.mainScreen, :frame
    frame = NSScreen.mainScreen.frame
    assert_kind_of CGRect, frame
    refute_equal 0, frame.size.width
    refute_equal 0, frame.size.height
  end

  # @todo how do we test this properly?
  def test_wakeup
    assert_respond_to NSScreen, :wakeup
    assert NSScreen.wakeup
  end

end
