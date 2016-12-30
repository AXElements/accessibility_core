require 'test/helper'
require 'accessibility/bridge'

class TestNSAttributedString < Minitest::Test

  def test_string
    string  = 'hi'
    astring = NSAttributedString.alloc.initWithString string
    10.times do
      assert_equal string, astring.string
    end
  end

  def test_length
    string  = 'hi'
    astring = NSAttributedString.alloc.initWithString string
    assert_equal string.length, astring.length
  end

end

