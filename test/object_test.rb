require 'test/helper'

class ObjectTest < MiniTest::Unit::TestCase

  if on_macruby?
    def test_to_ruby
      assert_same Object, Object.to_ruby
      assert_same 10, 10.to_ruby
    end

    def test_to_ax
      assert_same Object, Object.to_ax
      assert_same 10, 10.to_ax
    end
  end

  # we may have other tests for extensions to the Object class

end
