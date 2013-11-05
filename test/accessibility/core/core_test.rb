require 'test/accessibility/core/fixture'

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

  def pop_up
    @@pop_up ||= window_child 'AXPopUpButton'
  end

  def check_box
    @@check_box ||= window_child 'AXCheckBox'
  end

  def search_box
    @@search_box ||= window_child 'AXTextField'
  end

  def static_text
    @@static_text ||= window_child('AXStaticText') { |x|
      x.value.match /My Little Pony/
    }
  end

  def web_area
    @@web_area ||= window_child('AXScrollArea') { |x|
      x.attribute('AXDescription') == 'Test Web Area'
    }.children.first
  end

  def text_area
    @@text_area ||= window_child('AXScrollArea') { |x|
      x.attributes.include?('AXIdentifier') &&
      x.attribute('AXIdentifier') == 'Text Area'
    }.children.first
  end


  # @!group Tests for singleton methods

  def test_application_for
    assert_equal app, Accessibility::Element.application_for(PID)
    assert_raises(ArgumentError) { Accessibility::Element.application_for 0 }
  end

  def test_system_wide # depends on Element#role working... :/
    assert_equal 'AXSystemWide', Accessibility::Element.system_wide.role
  end

  def test_element_at_singleton
    [
     no_button.attribute('AXPosition'),
     no_button.attribute('AXPosition').to_a
    ].each do |point|

      element = Accessibility::Element.element_at point
      assert_equal no_button, element, "#{no_button.inspect} and #{element.inspect}"

      element = Accessibility::Element.element_at point
      assert_equal no_button, element, "#{no_button.inspect} and #{element.inspect}"

      assert_kind_of Accessibility::Element, element
    end

    # skip 'Need to find a way to guarantee an empty spot on the screen to return nil'
    # test manually for now :(
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

    data = window.attribute('AXData')
    assert_kind_of NSData, data
    assert data.length > 0

    assert_nil window.attribute('AXGrowArea'), 'KAXErrorNoValue == nil'

    assert_nil invalid_element.attribute('AXRole'), 'Dead element == nil'
    assert_nil invalid_element.attribute('AXChildren')
    assert_nil app.attribute('MADE_UP_ATTR')
  end

  def test_size_of
    assert_equal app.children.size, app.size_of('AXChildren')
    assert_equal 0,                 pop_up.size_of('AXChildren')
  end

  def test_size_of_dead_element_is_zero
    assert_equal 0, invalid_element.size_of('AXChildren'), 'Dead == 0'
  end

  def test_writable?
    refute app.writable?    'AXTitle'
    assert window.writable? 'AXMain'
  end

  def test_writable_always_false_for_dead_elements
    refute invalid_element.writable?('AXRole'), 'Dead is always false'
  end


  # @!group Test setting attribute values

  def test_set_number
    [25, 75, 50].each do |number|
      assert_equal number, slider.set('AXValue', number)
      assert_equal number, slider.value
    end
  end

  def test_set_string
    [Time.now.to_s, ''].each do |string|
      assert_equal string, search_box.set('AXValue', string)
      assert_equal string, search_box.value
    end
  end

  def test_set_point
    original_point = window.attribute 'AXPosition'
    [
     CGPoint.new(100, 100),
     CGPoint.new(256, 256)
    ].each do |point|
      assert_equal point, window.set('AXPosition', point)
      assert_equal point, window.attribute('AXPosition')
    end
  ensure # trololo
    window.set 'AXPosition', original_point
  end

  def test_set_range
    text_area.set 'AXValue', 'hey-o'

    text_area.set 'AXSelectedTextRange', 0..3
    assert_equal 0..3, text_area.attribute('AXSelectedTextRange')

    text_area.set 'AXSelectedTextRange', 1...4
    assert_equal 1..3, text_area.attribute('AXSelectedTextRange')
  ensure
    text_area.set 'AXValue', ''
  end

  def test_set_attr_handles_errors
    assert_raises(ArgumentError) { app.set 'FAKE', true }
    assert_raises(ArgumentError) { invalid_element.set 'AXTitle', 'hi' }
  end


  # @!group Tests for instance methods

  def test_role
    assert_equal 'AXApplication', app.role
    assert_equal 'AXWindow',      window.role
  end

  def test_subrole
    assert_equal 'AXStandardWindow', window.subrole
    assert_nil web_area.subrole
  end

  def test_parent
    assert_equal app, window.parent
    assert_nil app.parent
  end

  def test_children
    assert_equal app.attribute('AXChildren'), app.children
  end

  def test_children_returns_an_array_for_dead_elements
    assert_kind_of Array, invalid_element.children
  end

  def test_value
    assert_equal check_box.attribute('AXValue'), check_box.value
    assert_equal    slider.attribute('AXValue'), slider.value
  end

  def test_pid
    assert_equal PID, app.pid
    assert_equal PID, window.pid
    assert_equal 0, Accessibility::Element.system_wide.pid # special case
  end

  def test_application_for_pid_works # integration test
    assert_equal app, Accessibility::Element.application_for(app.pid)
    assert_equal app, Accessibility::Element.application_for(window.pid)
  end

  def test_parameterized_attributes
    assert_empty app.parameterized_attributes

    attrs = static_text.parameterized_attributes
    assert_includes attrs, 'AXStringForRange'
    assert_includes attrs, 'AXLineForIndex'
    assert_includes attrs, 'AXBoundsForRange'

    assert_empty invalid_element.parameterized_attributes, 'Dead should always be empty'
  end

  def test_parameterized_attribute
    expected = 'My Li'

    attr = static_text.parameterized_attribute('AXStringForRange', 0..4)
    assert_equal expected, attr

    attr = static_text.parameterized_attribute('AXAttributedStringForRange', 0..4)
    assert_equal expected, attr.string

    assert_nil invalid_element.parameterized_attribute('AXStringForRange', 0..0),
      'dead elements should return nil for any parameterized attribute'

    # Should add a test case to test the no value case, but it will have
    # to be fabricated in the test app.

    assert_raises(ArgumentError) {
      app.parameterized_attribute('AXStringForRange', 0..1)
    }
  end

  def test_actions
    assert_empty                   app.actions
    assert_equal ['AXPress'], yes_button.actions
  end

  def test_perform
    2.times do # twice so that it will reset
      val = check_box.value
      check_box.perform 'AXPress'
      refute_equal val, check_box.value
    end

    val  = slider.value
    slider.perform 'AXIncrement'
    assert slider.value > val

    val  = slider.value
    slider.perform 'AXDecrement'
    assert slider.value < val

    assert_raises(ArgumentError) { app.perform '' }
  end

  ##
  # The keyboard simulation stuff is a bit weird, we need to
  # hardcode some values because we cannot depend on the class
  # that translates strings into characters.
  #
  # This test also relies on being able to read and set the value
  # of an element using other methods in the class; thus it is
  # an integration test.
  def test_post
    events = [[0x56,true], [0x56,false], [0x54,true], [0x54,false]]
    string = '42'

    search_box.set 'AXFocused', true
    app.post events

    assert_equal string, search_box.value

  ensure # reset for next test
    search_box.set 'AXValue', ''
  end

  def test_invalid?
    assert_equal false, app.invalid?
    assert_equal true,  invalid_element.invalid?
    assert_equal false, window.invalid?
  end

  def test_set_timeout_to
    assert_equal 10, app.set_timeout_to(10)
    assert_equal 0,  app.set_timeout_to(0)
  end

  def test_application
    assert_equal app, app.application
    assert_equal app, window.application
  end

  def test_element_at
    [
     no_button.attribute('AXPosition'),
     no_button.attribute('AXPosition').to_a
    ].each do |point|

      element = app.element_at point
      assert_equal no_button, element, "#{no_button.inspect} and #{element.inspect}"

      element = Accessibility::Element.system_wide.element_at point
      assert_equal no_button, element, "#{no_button.inspect} and #{element.inspect}"

      assert_kind_of Accessibility::Element, element
    end

    # skip 'Need to find a way to guarantee an empty spot on the screen to return nil'
    # test manually for now :(
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


  # @!group Bugs

  # this assumes that radar://10040865 is not fixed;
  # once it is fixed this case becomes less of an
  # issue anyways
  #
  # @todo this is fixed on Sea Lion, so we need to implement the test another way
  def test_nil_children_returns_empty_array
    skip if on_sea_lion?
    menu = app.attribute('AXMenuBar').children.find { |child|
      child.attribute('AXTitle') =='Help'
    }
    menu.perform 'AXPress'

    assert_empty menu.children.first.children.first.children.first.children
  ensure
    menu.perform 'AXCancel' if menu
  end

end
