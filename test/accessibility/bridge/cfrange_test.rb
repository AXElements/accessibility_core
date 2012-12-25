require 'test/helper'

if on_macruby?
class CFRangeTest < MiniTest::Unit::TestCase

  def test_to_ax
    range = CFRange.new(5, 4)
    value = range.to_ax
    ptr   = Pointer.new CFRange.type
    AXValueGetValue(value, 4, ptr)
    assert_equal range, ptr.value, 'range makes a value'
  end

  def test_cf_range_to_ruby_range_compatability
    assert_equal CFRangeMake(1,10).to_ax, (1..10  ).to_ax
    assert_equal CFRangeMake(1, 9).to_ax, (1...10 ).to_ax
    assert_equal CFRangeMake(0, 3).to_ax, (0..2   ).to_ax
  end

end
end
