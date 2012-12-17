require 'test/helper'

##
# Some of these tests are unit tests and some of these tests are
# integration tests. In general I try to make the tests as isolated
# as possible, but some methods are impossible to meaningfully test
# without involving other parts of the same class.
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

  def yes_button
    @@yes_button ||= window_child('AXButton') { |x|
      x.attribute('AXTitle') == 'Yes'
    }
  end

  def no_button
    @@no_button ||= window_child('AXButton') { |x|
      x.attribute('AXTitle') == 'No'
    }
  end

  def bye_button
    @@bye_button ||= window_child('AXButton') { |x|
      x.attribute('AXTitle') == 'Bye!'
    }
  end

  def invalid_element
    bye_button # guarantee that it is cached
    @@dead ||= no_button.perform 'AXPress'
    bye_button
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

  ##
  # AFAICT every accessibility object **MUST** have attributes, so
  # there are no tests to check what happens when they do not exist;
  # though I am quite sure that AXElements will raise an exception.
  def test_attributes
    attrs = app.attributes
    assert_includes attrs, 'AXRole'
    assert_includes attrs, 'AXChildren'
    assert_includes attrs, 'AXTitle'
    assert_includes attrs, 'AXMenuBar'
  end

  def test_attributes_of_dead_element
    assert_empty invalid_element.attributes, 'Dead elements should have no attrs'
  end

  def test_attribute
    assert_equal 'AXElementsTester',  window.attribute('AXTitle')
    assert_equal false,               window.attribute('AXFocused')
    assert_equal CGSize.new(555,529), window.attribute('AXSize')
    assert_equal app,                 window.attribute('AXParent')
    assert_equal 10..19,              window.attribute('AXPie') # custom attribute!

    assert_nil window.attribute('AXGrowArea'), 'KAXErrorNoValue == nil'

    assert_nil invalid_element.attribute('AXRole'), 'Dead element == nil'
    assert_nil invalid_element.attribute('AXChildren')
    assert_nil app.attribute('MADE_UP_ATTR')
  end

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
