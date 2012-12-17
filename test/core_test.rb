require 'test/helper'

class CoreTest < MiniTest::Unit::TestCase

  def app
    APP
  end

  def test_application_for
    assert_equal app, Accessibility::Element.application_for(PID)
    assert_raises(ArgumentError) { Accessibility::Element.application_for 0 }
  end

  def test_equality_when_not_equal
    refute_equal app, 42
    refute_equal app, 3.14
    refute_equal app, 'not an element'
    refute_equal app, 1..2
    refute_equal app, CGPoint.new
  end

end
