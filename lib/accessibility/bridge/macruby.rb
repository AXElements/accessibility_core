require 'accessibility/bridge/common'

##
# accessibility-core extensions for `NSURL`
class NSURL
  ##
  # Return the reciver, for the receiver is already a URL object
  #
  # @return [NSURL]
  def to_url
    self
  end

  # because printing is easier this way
  alias_method :to_s, :inspect
end

##
# accessibility-core extensions for `NSString`
class NSString
  ##
  # Create an NSURL using the receiver as the initialization string
  #
  # If the receiver is not a valid URL then `nil` will be returned.
  #
  # This exists because of
  # [rdar://11207662](http://openradar.appspot.com/11207662).
  #
  # @return [NSURL,nil]
  def to_url
    NSURL.URLWithString self
  end
end

##
# `accessibility-core` extensions for `NSObject`
class NSObject
  ##
  # Return an object safe for passing to AXAPI
  def to_ax
    self
  end

  ##
  # Return a usable object from an AXAPI pointer
  def to_ruby
    self
  end
end

##
# `accessibility-core` extensions for `CFRange`
class CFRange
  ##
  # Convert the {CFRange} to a Ruby {Range} object
  #
  # @return [Range]
  def to_ruby
    Range.new location, (location + length - 1)
  end
end

##
# `accessibility-core` extensions for `Range`
class Range
  # @return [AXValueRef]
  def to_ax
    raise ArgumentError, "can't convert negative index" if last < 0 || first < 0
    length = if exclude_end?
               last - first
             else
               last - first + 1
             end
    CFRange.new(first, length).to_ax
  end
end

##
# AXElements extensions to the `Boxed` class
#
# The `Boxed` class is simply an abstract base class for structs that
# MacRuby can use via bridge support.
class Boxed
  ##
  # Returns the number that AXAPI uses in order to know how to wrap
  # a struct.
  #
  # @return [Number]
  def self.ax_value
    raise NotImplementedError, "#{inspect}:#{self.class} cannot be wrapped"
  end

  ##
  # Create an `AXValueRef` from the `Boxed` instance. This will only
  # work if for the most common boxed types, you will need to check
  # the AXAPI documentation for an up to date list.
  #
  # @example
  #
  #   CGPoint.new(12, 34).to_ax # => #<AXValueRef:0x455678e2>
  #   CGSize.new(56, 78).to_ax  # => #<AXValueRef:0x555678e2>
  #
  # @return [AXValueRef]
  def to_ax
    klass = self.class
    ptr   = Pointer.new klass.type
    ptr.assign self
    AXValueCreate(klass.ax_value, ptr)
  end
end

# `accessibility-core` extensions for `CFRange`'s metaclass
class << CFRange
  # (see Boxed.ax_value)
  def ax_value; KAXValueCFRangeType end
end
# `accessibility-core` extensions for `CGSize`'s metaclass
class << CGSize
  # (see Boxed.ax_value)
  def ax_value; KAXValueCGSizeType end
end
# `accessibility-core` extensions for `CGRect`'s metaclass
class << CGRect
  # (see Boxed.ax_value)
  def ax_value; KAXValueCGRectType end
end
# `accessibility-core` extensions for `CGPoint`'s metaclass
class << CGPoint
  # (see Boxed.ax_value)
  def ax_value; KAXValueCGPointType end
end

##
# Mixin for the special hidden class in MacRuby that represents `AXValueRef`
#
# This module adds a `#to_ruby` method that actually unwraps the object from
# the `AXValueRef` to some sort of {Boxed} object, such as a {CGPoint} or a
# {Range}.
module ValueWrapper


  ##
  # Map of type encodings used for unwrapping structs wrapped in an `AXValueRef`
  #
  # The list is order sensitive, which is why we unshift nil, but
  # should probably be more rigorously defined at runtime.
  #
  # @return [String,nil]
  BOX_TYPES = [CGPoint, CGSize, CGRect, CFRange].map!(&:type).unshift(nil)

  ##
  # Unwrap an `AXValueRef` into the `Boxed` instance that it is supposed
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

  # hack to find the __NSCFType class and mix things in
  klass = AXUIElementCreateSystemWide().class
  klass.send :include, self

end

##
# `accessibility-core` extensions to `NSArray`
class NSArray
  # @return [Array]
  def to_ruby
    map do |obj| obj.to_ruby end
  end
end

##
# `accessibility-core` extensions to `Object`
class Object

  ##
  # Spin the run loop for the given number of seconds
  #
  # The script will effectively be paused while the run loop
  # is "spinning".
  #
  # @param seconds [Number]
  def spin seconds
    CFRunLoopRunInMode(KCFRunLoopDefaultMode, seconds, false)
  end

end
