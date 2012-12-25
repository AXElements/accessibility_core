# -*- coding: utf-8 -*-

framework 'Cocoa'

# A workaround that guarantees that `CGPoint` is defined
MOUNTAIN_LION_APPKIT_VERSION ||= 1187
if NSAppKitVersionNumber >= MOUNTAIN_LION_APPKIT_VERSION
  framework '/System/Library/Frameworks/CoreGraphics.framework'
end

# check that the Accessibility APIs are enabled and are available to MacRuby
begin
  unless AXAPIEnabled()
    raise RuntimeError, <<-EOS
------------------------------------------------------------------------
Universal Access is disabled on this machine.

Please enable it in the System Preferences.
------------------------------------------------------------------------
    EOS
  end
rescue NoMethodError
  raise NotImplementedError, <<-EOS
------------------------------------------------------------------------
You need to install the latest BridgeSupport preview so that AXElements
has access to CoreFoundation.
------------------------------------------------------------------------
  EOS
end


##
# Core abstraction layer that that adds OO to the OS X Accessibility APIs
#
# This provides a generic object oriented mixin/class for the low level APIs.
# A more Ruby-ish wrapper is available through
# [AXElements](https://github.com/Marketcircle/AXElements).
#
# On MacRuby, bridge support turns C structs into "first class" objects. To
# that end, instead of adding an extra allocation to wrap the object, we will
# simply add a mixin to add some basic functionality. On MRI, a C extension
# encapsulates the structures into a class.
#
# For both Ruby platforms the interface should be the same. It is a bug if they
# are different.
#
# This module is responsible for handling pointers and dealing with error
# codes for functions that make use of them. The methods in this class
# provide a cleaner, more Ruby-ish interface to the low level CoreFoundation
# functions that compose AXAPI than are natively available.
#
# @example
#
#   element = Accessibility::Element.application_for pid_for_terminal
#   element.attributes                      # => ["AXRole", "AXChildren", ...]
#   element.size_of "AXChildren"            # => 2
#   element.children.first.role             # => "AXWindow"
#
module Accessibility::Element

  class << self

    # @!group Element Hierarchy Entry Points

    ##
    # Get the application object object for an application given the
    # process identifier (PID) for that application.
    #
    # @example
    #
    #   app = Element.application_for 54743  # => #<Accessibility::Element>
    #
    # @param pid [Number]
    # @return [Accessibility::Element]
    def application_for pid
      NSRunLoop.currentRunLoop.runUntilDate Time.now
      if NSRunningApplication.runningApplicationWithProcessIdentifier pid
        AXUIElementCreateApplication(pid)
      else
        raise ArgumentError, 'pid must belong to a running application'
      end
    end

    ##
    # Create a new reference to the system wide object
    #
    # This is very useful when working with the system wide object as
    # caching the system wide reference does not seem to work.
    #
    # @example
    #
    #   system_wide  # => #<Accessibility::Element>
    #
    # @return [Accessibility::Element]
    def system_wide
      AXUIElementCreateSystemWide()
    end

    ##
    # Find the top most element at the given point on the screen
    #
    # This is the same as {Accessibility::Element#element_at} except
    # that the check for the topmost element does not care which app
    # the element belongs to.
    #
    # The coordinates should be specified using the flipped coordinate
    # system (origin is in the top-left, increasing downward and to the right
    # as if reading a book in English).
    #
    # If more than one element is at the position then the z-order of the
    # elements will be used to determine which is "on top".
    #
    # This method will safely return `nil` if there is no UI element at the
    # give point.
    #
    # @example
    #
    #   Element.element_at [453, 200]             # table
    #   Element.element_at CGPoint.new(453, 200)  # table
    #
    # @param point [#to_point]
    # @return [Accessibility::Element,nil]
    def element_at point
      system_wide.element_at point
    end

    # @!endgroup


    ##
    # The delay between keyboard events used by {Accessibility::Element#post}
    #
    # The default value is `0.009` (`:normal`), which should be about 50
    # characters per second (down and up are separate events).
    #
    # This is just a magic number from trial and error. Both the repeat
    # interval (NXKeyRepeatInterval) and threshold (NXKeyRepeatThreshold)
    # were tried, but were way too big.
    #
    # @return [Number]
    attr_reader :key_rate

    ##
    # Set the delay between key events
    #
    # This value is used by {Accessibility::Element#post} to slow down the
    # typing speed so apps do not get overloaded by all the events arriving
    # at the same time.
    #
    # You can pass either an exact value for sleeping (a `Float` or
    # `Fixnum`), or you can use a preset symbol:
    #
    #  - `:very_slow`
    #  - `:slow`
    #  - `:normal`/`:default`
    #  - `:fast`
    #  - `:zomg`
    #
    # The `:zomg` setting will be too fast in almost all cases, but
    # it is fun to watch.
    #
    # @param value [Number,Symbol]
    def self.key_rate= value
      @key_rate = case value
                  when :very_slow        then 0.9
                  when :slow             then 0.09
                  when :normal, :default then 0.009
                  when :fast             then 0.0009
                  when :zomg             then 0.00009
                  else                        value.to_f
                  end
    end
  end
  @key_rate = 0.009


  # @!group Attributes

  ##
  # @todo Invalid elements do not always raise an error. This is a bug
  #       that should be logged with Apple (but I keep procrastinating).
  #
  # Get the list of attributes for the element
  #
  # As a convention, this method will return an empty array if the
  # backing element is no longer alive.
  #
  # @example
  #
  #   button.attributes # => ["AXRole", "AXRoleDescription", ...]
  #
  # @return [Array<String>]
  def attributes
    @attributes ||= (
      ptr  = Pointer.new ARRAY
      code = AXUIElementCopyAttributeNames(self, ptr)

      case code
      when 0                        then ptr.value
      when KAXErrorInvalidUIElement then []
      else handle_error code
      end
      )
  end

  ##
  # Fetch the value for the given attribute from the receiver
  #
  # CoreFoundation wrapped objects will be unwrapped for you, if you expect
  # to get a {CFRange} you will be given a {Range} instead.
  #
  # As a convention, if the backing element is no longer alive then
  # any attribute value will return `nil`. If the attribute is not supported
  # by the element then `nil` will be returned instead. These
  # conventions are debatably necessary, inquire for details.
  #
  # @example
  #   window.attribute 'AXTitle'    # => "HotCocoa Demo"
  #   window.attribute 'AXSize'     # => #<CGSize width=10.0 height=88>
  #   window.attribute 'AXParent'   # => #<Accessibility::Element>
  #   window.attribute 'AXHerpDerp' # => nil
  #
  # @param name [String]
  def attribute name
    ptr  = Pointer.new :id
    code = AXUIElementCopyAttributeValue(self, name, ptr)

    case code
    when 0
      ptr.value.to_ruby
    when KAXErrorFailure, KAXErrorNoValue,
         KAXErrorInvalidUIElement, KAXErrorAttributeUnsupported
      nil
    else
      handle_error code, name
    end
  end

  ##
  # @note It has been observed that some elements may lie with this value.
  #       Bugs should be reported to the app developers in those cases.
  #       I'm looking at you, Safari!
  #
  # Get the size of the array that would be returned by calling {#attribute}
  #
  # When performance matters, this is much faster than getting the array
  # and asking for the size.
  #
  # If there is a failure or the backing element is no longer alive, this
  # method will return `0`.
  #
  # @example
  #
  #   window.size_of 'AXChildren'  # => 19
  #   table.size_of  'AXRows'      # => 100
  #
  # @param name [String]
  # @return [Number]
  def size_of name
    ptr  = Pointer.new :long_long
    code = AXUIElementGetAttributeValueCount(self, name, ptr)

    case code
    when 0
      ptr.value
    when KAXErrorFailure, KAXErrorNoValue, KAXErrorInvalidUIElement
      0
    else
      handle_error code, name
    end
  end

  ##
  # Returns whether or not the given attribute is writable on the reciver
  #
  # Often, you will want/need to check writability of an attribute before
  # trying to call {Accessibility::Element#set} for the attribute.
  #
  # In case of internal error, or if the element dies, this method will
  # return `false`.
  #
  # @example
  #
  #   window.writable? 'AXSize'  # => true
  #   window.writable? 'AXTitle' # => false
  #
  # @param name [String]
  def writable? name
    ptr  = Pointer.new :bool
    code = AXUIElementIsAttributeSettable(self, name, ptr)

    case code
    when 0
      ptr.value
    when KAXErrorFailure, KAXErrorNoValue, KAXErrorInvalidUIElement
      false
    else
      handle_error code, name
    end
  end

  ##
  # Set the given value for the given attribute on the receiver
  #
  # You do not need to worry about wrapping objects first, `Range`
  # objects will also be automatically converted into `CFRange` objects
  # (unless they have a negative index) and then wrapped.
  #
  # This method does not check writability of the attribute you are
  # setting. If you need to check, use {Accessibility::Element#writable?}
  # first to check.
  #
  # Unlike when reading attributes, writing to a dead element, and
  # other error conditions, will raise an exception.
  #
  # @example
  #
  #   set 'AXValue',        "hi"       # => "hi"
  #   set 'AXSize',         [250,250]  # => [250,250]
  #   set 'AXVisibleRange', 0..3       # => 0..3
  #   set 'AXVisibleRange', 1...4      # => 1..3
  #
  # @param name [String]
  # @param value [Object]
  def set name, value
    code = AXUIElementSetAttributeValue(self, name, value.to_ax)
    if code.zero?
      value
    else
      handle_error code, name, value
    end
  end

  ##
  # Shortcut for getting the `"AXRole"` attribute
  #
  # The role of an element roughly translates to the class of the object;
  # however this can be thought of as the superclass of the object if the
  # object also has a `"AXSubrole"` attribute.
  #
  # Remember that dead elements may return `nil` for their role.
  #
  # @example
  #
  #   window.role  # => "AXWindow"
  #
  # @return [String,nil]
  def role
    attribute KAXRoleAttribute
  end

  ##
  # @note You might get `nil` back as the subrole even if the object claims
  #       to have a subrole attribute. AXWebArea objects are known to do this.
  #       You need to check. :(
  #
  # Shortcut for getting the `"AXSubrole"`
  #
  # The subrole of an element roughly translates to the class of the object,
  # but only if the object has a subrole. If an object does not have a subrole
  # then the class of the object would be the {#role}.
  #
  # @example
  #   window.subrole    # => "AXDialog"
  #   web_area.subrole  # => nil
  #
  # @return [String,nil]
  def subrole
    attribute KAXSubroleAttribute
  end

  ##
  # Shortcut for getting the `"AXParent"`
  #
  # The "parent" attribute of an element is the general way in which you would
  # navigate upwards through the hierarchy of the views in an app.
  #
  # An element will be returned if the receiver has a parent, otherwise `nil`
  # will be returned. Incorrectly implemented elements may also return `nil`.
  # Usually only something that has a {#role} of `"AXApplication"` will return
  # `nil` since it does not have a parent.
  #
  # @example
  #
  #   window.parent # => app
  #   app.parent # => nil
  #
  # @return [Accessibility::Element,nil]
  def parent
    ptr  = Pointer.new :id
    code = AXUIElementCopyAttributeValue(self, KAXParentAttribute, ptr)
    code.zero? ? ptr.value.to_ruby : nil
  end

  ##
  # Shortcut for getting the `"AXChildren"`
  #
  # The "children" attribute of an element is the general way in which you would
  # navigate downwards through the hierarchy of the views in an app.
  #
  # An array will always be returned, even if the element is dead or has no
  # children (but the array will be empty in those cases).
  #
  # @example
  #
  #   app.children # => [MenuBar, Window, ...]
  #
  # @return [Array<Accessibility::Element>]
  def children
    ptr  = Pointer.new :id
    code = AXUIElementCopyAttributeValue(self, KAXChildrenAttribute, ptr)
    code.zero? ? ptr.value.to_ruby : []
  end

  ##
  # Shortcut for getting the `"AXValue"`
  #
  # @example
  #
  #   label.value   # => "Mark Rada"
  #   slider.value  # => 42
  #
  def value
    attribute KAXValueAttribute
  end

  ##
  # Get the process identifier (PID) of the application of the receiver
  #
  # This method will return `0` if the element is dead or if the receiver
  # is the the system wide element.
  #
  # @example
  #
  #   window.pid               # => 12345
  #   Element.system_wide.pid  # => 0
  #
  # @return [Fixnum]
  def pid
    @pid ||= (
      ptr  = Pointer.new :int
      code = AXUIElementGetPid(self, ptr)

      case code
      when 0
        ptr.value
      when KAXErrorInvalidUIElement
        self == Accessibility::Element.system_wide ? 0 : handle_error(code)
      else
        handle_error code
      end
      )
  end


  # @!group Parameterized Attributes

  ##
  # Get the list of parameterized attributes for the element
  #
  # Similar to {#attributes}, this method will also return an empty
  # array if the element is dead.
  #
  # Most elements do not have parameterized attributes, but the ones
  # that do, have many.
  #
  # @example
  #
  #   text_area.parameterized_attributes  # => ["AXStringForRange", ...]
  #   app.parameterized_attributes        # => []
  #
  # @return [Array<String>]
  def parameterized_attributes
    @parameterized_attributes ||= (
      ptr  = Pointer.new ARRAY
      code = AXUIElementCopyParameterizedAttributeNames(self, ptr)

      case code
      when 0                                         then ptr.value
      when KAXErrorNoValue, KAXErrorInvalidUIElement then []
      else handle_error code
      end
      )
  end

  ##
  # Fetch the given pramaeterized attribute value for the given parameter
  #
  # Low level objects, such as {Accessibility::Element} and {Boxed} objects,
  # will be unwrapped for you automatically and {CFRange} objects will be
  # turned into {Range} objects. Similarly, you do not need to worry about
  # wrapping the parameter as that will be done for you (except for {Range}
  # objects that use a negative index).
  #
  # As a convention, if the backing element is no longer alive, or the
  # attribute does not exist, or a system failure occurs then you will
  # receive `nil`.
  #
  # @example
  #
  #   parameterized_attribute KAXStringForRangeParameterizedAttribute, 1..10
  #     # => "ello, worl"
  #
  # @param name [String]
  # @param param [Object]
  def parameterized_attribute name, param
    ptr  = Pointer.new :id
    code = AXUIElementCopyParameterizedAttributeValue(self, name, param.to_ax, ptr)

    case code
    when 0
      ptr.value.to_ruby
    when KAXErrorFailure, KAXErrorNoValue, KAXErrorInvalidUIElement
      nil
    else
      handle_error code, name, param
    end
  end


  # @!group Actions

  ##
  # Get the list of actions that the element can perform
  #
  # If an element does not have actions, then an empty list will be
  # returned. Dead elements will also return an empty array.
  #
  # @example
  #
  #   button.actions  # => ["AXPress"]
  #
  # @return [Array<String>]
  def actions
    @actions ||= (
      ptr  = Pointer.new ARRAY
      code = AXUIElementCopyActionNames(self, ptr)

      case code
      when 0                        then ptr.value
      when KAXErrorInvalidUIElement then []
      else handle_error code
      end
      )
  end

  ##
  # Ask the receiver to perform the given action
  #
  # This method will always return true or raises an exception. Actions
  # should never fail, but there are some extreme edge cases (e.g. out
  # of memory, etc.).
  #
  # Unlike when reading attributes, performing an action on a dead element
  # will raise an exception.
  #
  # @example
  #
  #   perform KAXPressAction  # => true
  #
  # @param action [String]
  # @return [Boolean]
  def perform action
    code = AXUIElementPerformAction(self, action)
    if code.zero?
      true
    else
      handle_error code, action
    end
  end

  ##
  # Post the list of given keyboard events to the receiver
  #
  # This only applies if the given element is an application object or
  # the system wide object. The focused element will receive the events.
  #
  # Events could be generated from a string using output from the
  # `accessibility_keyboard` gem's `Accessibility::String#keyboard_events_for`
  # method.
  #
  # Events are number/boolean tuples, where the number is a keycode
  # and the boolean is the keypress state (true is keydown, false is
  # keyup).
  #
  # You can learn more about keyboard events from the
  # [Keyboard Events documentation](http://github.com/Marketcircle/AXElements/wiki/Keyboarding).
  #
  # @example
  #
  #   include Accessibility::String
  #   events = keyboard_events_for "Hello, world!\n"
  #   app.post events
  #
  # @param events [Array<Array(Number,Boolean)>]
  # @return [self]
  def post events
    events.each do |event|
      code = AXUIElementPostKeyboardEvent(self, 0, *event)
      handle_error code unless code.zero?
      sleep Accessibility::Element.key_rate
    end
    sleep 0.1 # in many cases, UI is not done updating right away
    self
  end


  # @!group Misc.

  ##
  # Return whether or not the receiver is "dead"
  #
  # A dead element is one that is no longer in the app's view
  # hierarchy. This is not the same as visibility; an element that is
  # invalid will not be visible, but an invisible element might still
  # be valid (it depends on the clients implementation of the API).
  def invalid?
    AXUIElementCopyAttributeValue(self, KAXRoleAttribute, Pointer.new(:id)) ==
      KAXErrorInvalidUIElement
  end

  ##
  # Change the timeout value for the element
  #
  # The timeout value is mostly effective for apps that are slow to respond to
  # accessibility queries, or if you intend to make a large query (such as thousands
  # of rows in a table).
  #
  # If you change the timeout on the system wide object, it affets all timeouts.
  #
  # Setting the global timeout to `0` seconds will reset the timeout value
  # to the system default. The system default timeout value is `6 seconds`
  # as of the writing of this documentation, but Apple has not publicly
  # documented this (we had to ask in person at WWDC).
  #
  # @param seconds [Number]
  # @return [Number]
  def set_timeout_to seconds
    case code = AXUIElementSetMessagingTimeout(self, seconds)
    when 0 then seconds
    else handle_error code, seconds
    end
  end

  ##
  # Returns the application reference (toplevel element) for the receiver
  #
  # @return [Accessibility::Element]
  def application
    Accessibility::Element.application_for pid
  end


  # @!group Element Hierarchy Entry Points

  ##
  # Find the topmost element at the given point for the receiver's app
  #
  # If the receiver is the system wide object then the return is the
  # topmost element regardless of application.
  #
  # The coordinates should be specified using the flipped coordinate
  # system (origin is in the top-left, increasing downward and to the right
  # as if reading a book in English).
  #
  # If more than one element is at the position then the z-order of the
  # elements will be used to determine which is "on top".
  #
  # This method will safely return `nil` if there is no UI element at the
  # give point.
  #
  # @example
  #
  #   Element.system_wide.element_at [453, 200]  # table
  #   app.element_at CGPoint.new(453, 200)       # table
  #
  # @param point [#to_point]
  # @return [Accessibility::Element,nil]
  def element_at point
    ptr  = Pointer.new ELEMENT
    code = AXUIElementCopyElementAtPosition(self, *point.to_point, ptr)

    case code
    when 0
      ptr.value.to_ruby
    when KAXErrorNoValue
      nil
    when KAXErrorInvalidUIElement # @todo uhh, why is this here again?
      unless self == Accessibility::Element.system_wide
        Accessibility::Element.element_at point
      end
    else
      handle_error code, point, nil, nil
    end
  end


  private

  # @!group Error Handling

  ##
  # @private
  #
  # Mapping of `AXError` values to static information on how to handle
  # the error. Used by {handle_error}.
  #
  # @return [Hash{Number=>Array(Symbol,Range)}]
  AXERROR = {
    KAXErrorFailure                           => [
      RuntimeError,
      lambda { |*args|
        "A system failure occurred with #{args[0].inspect}, stopping to be safe"
      }
    ],
    KAXErrorIllegalArgument                   => [
      ArgumentError,
      lambda { |*args|
        case args.size
        when 1
          "#{args[0].inspect} is not an Accessibility::Element"
        when 2
          "Either the element #{args[0].inspect} or the attribute/action" +
            "#{args[1].inspect} is not a legal argument"
        when 3
          "You can't get/set #{args[1].inspect} with/to #{args[2].inspect} " +
            "for #{args[0].inspect}"
        when 4
          "The point #{args[1].to_point.inspect} is not a valid point, " +
            "or #{args[0].inspect} is not an Accessibility::Element"
        end
      }
    ],
    KAXErrorInvalidUIElement                  => [
      ArgumentError,
      lambda { |*args|
        "#{args[0].inspect} is no longer a valid reference"
      }
    ],
    KAXErrorInvalidUIElementObserver          => [
      ArgumentError,
      lambda { |*args|
        'AXElements no longer supports notifications'
      }
    ],
    KAXErrorCannotComplete                    => [
      RuntimeError,
      lambda { |*args|
        NSRunLoop.currentRunLoop.runUntilDate Time.now # spin the run loop once
        pid = args[0].pid
        app = NSRunningApplication.runningApplicationWithProcessIdentifier pid
        if app
          "An unspecified error occurred using #{args[0].inspect} with AXAPI, maybe a timeout :("
        else
          "Application for pid=#{pid} is no longer running. Maybe it crashed?"
        end
      }
    ],
    KAXErrorAttributeUnsupported              => [
      ArgumentError,
      lambda { |*args|
        "#{args[0].inspect} does not have a #{args[1].inspect} attribute"
      }
    ],
    KAXErrorActionUnsupported                 => [
      ArgumentError,
      lambda { |*args|
        "#{args[0].inspect} does not have a #{args[1].inspect} action"
      }
    ],
    KAXErrorNotificationUnsupported           => [
      ArgumentError,
      lambda { |*args|
        'AXElements no longer supports notifications'
      }
    ],
    KAXErrorNotImplemented                    => [
      NotImplementedError,
      lambda { |*args|
        "The program that owns #{args[0].inspect} does not work with AXAPI properly"
      }
    ],
    KAXErrorNotificationAlreadyRegistered     => [
      ArgumentError,
      lambda { |*args|
        'AXElements no longer supports notifications'
      }
    ],
    KAXErrorNotificationNotRegistered         => [
      RuntimeError,
      lambda { |*args|
        'AXElements no longer supports notifications'
      }
    ],
    KAXErrorAPIDisabled                       => [
      RuntimeError,
      lambda { |*args|
        'AXAPI has been disabled'
      }
    ],
    KAXErrorNoValue                           => [
      RuntimeError,
      lambda { |*args|
        'AXElements internal error. ENoValue should be handled internally!'
      }
    ],
    KAXErrorParameterizedAttributeUnsupported => [
      ArgumentError,
      lambda { |*args|
        "#{args[0].inspect} does not have a #{args[1].inspect} parameterized attribute"
      }
    ],
    KAXErrorNotEnoughPrecision                => [
      RuntimeError,
      lambda { |*args|
        'AXAPI said there was not enough precision ¯\(°_o)/¯'
      }
    ]
  }

  # @param code [Number]
  def handle_error code, *args
    raise RuntimeError, 'assertion failed: code 0 means success!' if code.zero?
    klass, handler = AXERROR.fetch code, [
      RuntimeError,
      lambda { |*args| "An unknown error code was returned [#{code}]:#{inspect}" }
    ]
    raise klass, handler.call(self, *args), caller(1)
  end


  # @!endgroup


  ##
  # @private
  #
  # `Pointer` type encoding for `CFArrayRef` objects
  #
  # @return [String]
  ARRAY    = '^{__CFArray}'

  ##
  # @private
  #
  # `Pointer` type encoding for `AXUIElementRef` objects
  #
  # @return [String]
  ELEMENT  = '^{__AXUIElement}'

end

# hack to find the __NSCFType class and mix things in
klass = AXUIElementCreateSystemWide().class
klass.send :include, Accessibility::Element
