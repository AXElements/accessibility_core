require 'test/helper'
require 'lib/accessibility/highlighter'

class NSColorTest < MiniTest::Unit::TestCase

  def test_equality
    assert_equal NSColor.redColor, NSColor.redColor
    assert_equal NSColor.greenColor, NSColor.greenColor
    assert_equal NSColor.blueColor, NSColor.blueColor
    refute_equal NSColor.blueColor, NSColor.redColor
    refute_equal NSColor.greenColor, 1
  end

end
