require 'test/helper'
require 'accessibility/bridge'

if on_macruby?

##
# Boxed is the common ancestor of all structs defined by bridge support
# in MacRuby.
class BoxedTest < MiniTest::Unit::TestCase

  def test_to_ax_raises_for_arbitrary_boxes
    assert_raises(NotImplementedError) { NSEdgeInsets.new.to_ax }
  end

  def test_to_ruby # bet you would never dare to abuse the parser like this!
    assert_equal CGPointZero,         CGPointZero        .to_ax.to_ruby
    assert_equal CGSize.new(10,10),   CGSize.new(10, 10) .to_ax.to_ruby
    assert_equal CGRectMake(1,2,3,4), CGRectMake(1,2,3,4).to_ax.to_ruby
    assert_equal 1..10,               CFRange.new(1, 10) .to_ax.to_ruby
    assert_equal Range.new(1,10),     CFRange.new(1,10)  .to_ax.to_ruby
  end

end
end
