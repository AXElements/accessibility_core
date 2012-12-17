require 'test/helper'

class CoreTest < MiniTest::Unit::TestCase

  # @!group Helpers

  def app
    APP
  end

  def window
    @@window ||= app.attribute('AXWindows').first
  end

  def window_child name
    window.children.find { |item|
      if item.role == name
        block_given? ? yield(item) : true
      end
    }
  end

  def slider
    @@slider ||= window_child 'AXSlider'
  end


  # @!group Tests for singleton methods

  def test_application_for
    assert_equal app, Accessibility::Element.application_for(PID)
    assert_raises(ArgumentError) { Accessibility::Element.application_for 0 }
  end

  def test_system_wide # depends on Element#role working... :/
    assert_equal 'AXSystemWide', Accessibility::Element.system_wide.role
  end

  def test_key_rate
    assert_equal 0.009, Accessibility::Element.key_rate
    [
      [0.9,     :very_slow],
      [0.09,    :slow],
      [0.009,   :normal],
      [0.0009,  :fast],
      [0.00009, :zomg]
    ].each do |num, name|
      Accessibility::Element.key_rate = name
      assert_equal num, Accessibility::Element.key_rate
    end
  ensure
    Accessibility::Element.key_rate = 0.009
  end


  # @!group Tests for instance methods

  def test_equality
    assert_equal window, window
    assert_equal slider, slider
  end

  def test_equality_when_not_equal
    refute_equal app, 42
    refute_equal app, 3.14
    refute_equal app, 'not an element'
    refute_equal app, 1..2
    refute_equal app, CGPoint.new
  end

end
