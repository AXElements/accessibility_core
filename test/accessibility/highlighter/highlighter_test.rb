require 'test/helper'
require 'accessibility/highlighter'
require 'accessibility/extras'

class HighlighterTest < Minitest::Test
  try_to_parallelize!

  def teardown
    @w.stop if @w
  end

  def bounds
    [100, 100, 100, 100].to_rect
  end

  def test_highlight_returns_created_window
    @w = Accessibility::Highlighter.new bounds
    assert_kind_of Accessibility::Highlighter, @w
    assert_respond_to @w, :stop
  end

  def test_highlighter_stop_visible?
    @w = Accessibility::Highlighter.new bounds
    @w.stop
    refute @w.visible?
  end

  def test_highlight_highlights_correct_frame
    @w = Accessibility::Highlighter.new bounds
    assert_equal @w.frame, bounds.flip!
  end

  def test_highlight_with_timeout
    @w = Accessibility::Highlighter.new bounds, timeout: 0.1
    assert @w.visible?
    sleep 0.15
    refute @w.visible? # Not exactly the assertion I want, but close enough
  end

  def test_highlight_color
    @w = Accessibility::Highlighter.new bounds, color: NSColor.cyanColor
    assert_equal @w.color, NSColor.cyanColor
    @w.stop

    # test both spellings of colour
    @w = Accessibility::Highlighter.new bounds, colour: NSColor.purpleColor
    assert_equal @w.color, NSColor.purpleColor
    @w.stop
  end

  def test_highlight_random_pattern
    colors = [
              NSColor.redColor,
              NSColor.greenColor,
              NSColor.blueColor,
              NSColor.purpleColor,
              NSColor.yellowColor,
              NSColor.orangeColor,
              ]
    range = NSScreen.mainScreen.frame.size.height
    @highlighters = Array.new 100 do
      bounds = Array.new(2) { rand(range) } + Array.new(2) { rand(range) + 20 }
      color  = colors.sample
      w      = Accessibility::Highlighter.new bounds, color: color
      assert_equal bounds.to_rect.flip!, w.frame
      assert_equal color,                w.color
      w
    end
  ensure
    @highlighters.map(&:stop) if @highlighters
  end

end
