# -*- coding: utf-8 -*-

framework 'Cocoa'

# A workaround that guarantees that `CGPoint` is defined
MOUNTAIN_LION_APPKIT_VERSION = 1187
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


require 'accessibility/core/core_ext/macruby'


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
    def self.application_for pid
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
    # @param [Number,Symbol]
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
  # Fetch the value for an attribute. CoreFoundation wrapped objects
  # will be unwrapped for you, if you expect to get a {CFRange} you
  # will be given a {Range} instead.
  #
  # As a convention, if the backing element is no longer alive then
  # any attribute value will return `nil`, except for `KAXChildrenAttribute`
  # which will return an empty array. This is a debatably necessary evil,
  # inquire for details.
  #
  # If the attribute is not supported by the element then a exception
  # will be raised.
  #
  # @example
  #   window.attribute KAXTitleAttribute    # => "HotCocoa Demo"
  #   window.attribute KAXSizeAttribute     # => #<CGSize width=10.0 height=88>
  #   window.attribute KAXParentAttribute   # => #<AXUIElementRef>
  #   window.attribute KAXNoValueAttribute  # => nil
  #
  # @param name [String]
  def attribute name
    ptr  = Pointer.new :id
    code = AXUIElementCopyAttributeValue(self, name, ptr)

    case code
    when 0
      ptr.value.to_ruby
    when KAXErrorNoValue, KAXErrorInvalidUIElement
      nil
    else
      handle_error code, name
    end
  end

  ##
  # Shortcut for getting the `KAXRoleAttribute`. Remember that
  # dead elements may return `nil` for their role.
  #
  # @example
  #
  #   window.role  # => KAXWindowRole
  #
  # @return [String,nil]
  def role
    attribute KAXRoleAttribute
  end

  ##
  # @note You might get `nil` back as the subrole as AXWebArea
  #       objects are known to do this. You need to check. :(
  #
  # Shortcut for getting the `KAXSubroleAttribute`.
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
  # Shortcut for getting the `KAXChildrenAttribute`. An exception will
  # be raised if the object does not have children.
  #
  # @example
  #
  #   app.children # => [MenuBar, Window, ...]
  #
  # @return [Array<AXUIElementRef>]
  def children
    ptr  = Pointer.new :id
    code = AXUIElementCopyAttributeValue(self, KAXChildrenAttribute, ptr)
    code.zero? ? ptr.value.to_ruby : []
  end

  ##
  # Shortcut for getting the `KAXValueAttribute`.
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
  # Get the process identifier (PID) of the application that the element
  # belongs to.
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

  ##
  # Return whether or not the receiver is "dead".
  #
  # A dead element is one that is no longer in the app's view
  # hierarchy. This is not the same as visibility; an element that is
  # invalid will not be visible, but an invisible element might still
  # be valid.
  def invalid?
    AXUIElementCopyAttributeValue(self, KAXRoleAttribute, Pointer.new(:id)) ==
      KAXErrorInvalidUIElement
  end

  ##
  # @note It has been observed that some elements may lie with this value.
  #       Bugs should be reported to the app developers in those cases.
  #
  # Get the size of the array for attributes that would return an array.
  # When performance matters, this is much faster than getting the array
  # and asking for the size.
  #
  # If there is a failure or the backing element is no longer alive, this
  # method will return `0`.
  #
  # @example
  #
  #   window.size_of KAXChildrenAttribute  # => 19
  #   table.size_of KAXRowsAttribute       # => 100
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
  # Returns whether or not an attribute is writable. Often, you will
  # want/need to check writability of an attribute before trying call
  # {#set} for the attribute.
  #
  # In case of internal error or if the element dies, this method will
  # return `false`.
  #
  # @example
  #
  #   window.writable? KAXSizeAttribute  # => true
  #   window.writable? KAXTitleAttribute # => false
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
  # Set the given value for the given attribute. You do not need to
  # worry about wrapping objects first, `Range` objects will also
  # be automatically converted into `CFRange` objects (unless they
  # have a negative index) and then wrapped.
  #
  # This method does not check writability of the attribute you are
  # setting. If you need to check, use {#writable?} first to check.
  #
  # Unlike when reading attributes, writing to a dead element, and
  # other error conditions, will raise an exception.
  #
  # @example
  #
  #   set KAXValueAttribute,        "hi"       # => "hi"
  #   set KAXSizeAttribute,         [250,250]  # => [250,250]
  #   set KAXVisibleRangeAttribute, 0..3       # => 0..3
  #   set KAXVisibleRangeAttribute, 1...4      # => 1..3
  #
  # @param name [String]
  def set name, value
    code = AXUIElementSetAttributeValue(self, name, value.to_ax)
    if code.zero?
      value
    else
      handle_error code, name, value
    end
  end


  # @!group Parameterized Attributes

  ##
  # Get the list of parameterized attributes for the element. If the
  # element does not have parameterized attributes, then an empty
  # list will be returned.
  #
  # Most elements do not have parameterized attributes, but the ones
  # that do, have many.
  #
  # Similar to {#attributes}, this method will also return an empty
  # array if the element is dead.
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
  # Fetch the given pramaeterized attribute value for the given parameter.
  # Low level objects, such as `AXUIElementRef` and {Boxed} objects, will
  # be unwrapped for you automatically and {CFRange} objects will be turned
  # into {Range} objects. Similarly, you do not need to worry about wrapping
  # the parameter as that will be done for you (except for {Range} objects
  # that use a negative index).
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
  # Get the list of actions that the element can perform. If an element
  # does not have actions, then an empty list will be returned. Dead
  # elements will also return an empty array.
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
  # Ask an element to perform the given action. This method will always
  # return true or raise an exception. Actions should never fail, but
  # there are some extreme edge cases (e.g. out of memory, etc.).
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
  # Post the list of given keyboard events to the element. This only
  # applies if the given element is an application object or the
  # system wide object. The focused element will receive the events.
  #
  # Events could be generated from a string using output from
  # {Accessibility::String#keyboard_events_for}.
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


  # @!group Element Hierarchy Entry Points

  ##
  # Find the top most element at a point on the screen that belongs to the
  # backing application. If the backing element is the system wide object
  # then the return is the top most element regardless of application.
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
  # @return [AXUIElementRef,nil]
  def element_at point
    ptr  = Pointer.new ELEMENT
    code = AXUIElementCopyElementAtPosition(self, *point.to_point, ptr)

    case code
    when 0
      ptr.value.to_ruby
    when KAXErrorNoValue
      nil
    when KAXErrorInvalidUIElement
      unless self == Accessibility::Element.system_wide
        Accessibility::Element.system_wide.element_at point
      end
    else
      handle_error code, point, nil, nil
    end
  end


  # @!group Misc.

  ##
  # Returns the application reference for the application that the receiver
  # belongs to.
  #
  # @return [AXUIElementRef]
  def application
    Accessibility::Element.application_for pid
  end

  ##
  # Unwrap an `AXValue` into the `Boxed` instance that it is supposed
  # to be. This will only work for the most common boxed types, you will
  # need to check the AXAPI documentation for an up to date list.
  #
  # @example
  #
  #   wrapped_point.to_ruby # => #<CGPoint x=44.3 y=99.0>
  #   wrapped_range.to_ruby # => #<CFRange begin=7 length=100>
  #   wrapped_thing.to_ruby # => wrapped_thing
  #
  # @return [Boxed]
  def to_ruby
    type = AXValueGetType(self)
    return self if type.zero?

    ptr = Pointer.new BOX_TYPES[type]
    AXValueGetValue(self, type, ptr)
    ptr.value.to_ruby
  end


  # @!group Debug

  ##
  # Change the timeout value for the element. If you change the timeout
  # on the system wide object, it affets all timeouts.
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
          "#{args[0].inspect} is not an AXUIElementRef"
        when 2
          "Either the element #{args[0].inspect} or the attribute/action" +
            "#{args[1].inspect} is not a legal argument"
        when 3
          "You can't get/set #{args[1].inspect} with/to #{args[2].inspect} " +
            "for #{args[0].inspect}"
        when 4
          "The point #{args[1].to_point.inspect} is not a valid point, " +
            "or #{args[0].inspect} is not an AXUIElementRef"
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
  # `Pointer` type encoding for `CFArrayRef` objects.
  #
  # @return [String]
  ARRAY    = '^{__CFArray}'

  ##
  # @private
  #
  # `Pointer` type encoding for `AXUIElementRef` objects.
  #
  # @return [String]
  ELEMENT  = '^{__AXUIElement}'

  ##
  # Map of type encodings used for wrapping structs when coming from
  # an `AXValueRef`.
  #
  # The list is order sensitive, which is why we unshift nil, but
  # should probably be more rigorously defined at runtime.
  #
  # @return [String,nil]
  BOX_TYPES = [CGPoint, CGSize, CGRect, CFRange].map!(&:type).unshift(nil)

end

# hack to find the __NSCFType class and mix things in
klass = AXUIElementCreateSystemWide().class
klass.send :include, Accessibility::Element
