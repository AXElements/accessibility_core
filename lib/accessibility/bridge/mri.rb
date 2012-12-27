##
# A structure that contains a point in a two-dimensional coordinate system
CGPoint = Struct.new(:x, :y) unless defined? CGPoint
class CGPoint

  # @param x [Number]
  # @param y [Number]
  def initialize x = 0.0, y = 0.0
    super x.to_f, y.to_f
  end

  # @!attribute [rw] x
  #   The `x` co-ordinate of the screen point
  #   @return [Float]

  # @!attribute [rw] y
  #   The `y` co-ordinate of the screen point
  #   @return [Float]

  ##
  # Return a nice string representation of the point
  #
  # Overrides `Object#inspect` to more closely mimic MacRuby `Boxed#inspect`.
  #
  # @return [String]
  def inspect
    "#<CGPoint x=#{self.x.to_f} y=#{self.y.to_f}>"
  end

end


##
# A structure that contains the size of a rectangle in a 2D co-ordinate system
CGSize = Struct.new(:width, :height) unless defined? CGSize
class CGSize

  # @param width [Number]
  # @param height [Number]
  def initialize width = 0.0, height = 0.0
    super width.to_f, height.to_f
  end

  # @!attribute [rw] width
  #   The `width` of the box
  #   @return [Float]

  # @!attribute [rw] height
  #   The `heighth` of the box
  #   @return [Float]

  ##
  # Return a nice string representation of the size
  #
  # Overrides `Object#inspect` to more closely mimic MacRuby `Boxed#inspect`.
  #
  # @return [String]
  def inspect
    "#<CGSize width=#{self.width.to_f} height=#{self.height.to_f}>"
  end

end


##
# Complete definition of a rectangle in a 2D coordinate system
CGRect = Struct.new(:origin, :size) unless defined? CGRect
class CGRect

  # @param origin [CGPoint,#to_point]
  # @param size [CGSize,#to_size]
  def initialize origin = CGPoint.new, size = CGSize.new
    super(origin.to_point, size.to_size)
  end

  # @!attribute [rw] origin
  #   The `origin` point
  #   @return [CGPoint,#to_point]

  # @!attribute [rw] size
  #   The `size` of the rectangle
  #   @return [CGSize,#to_size]

  ##
  # Return a nice string representation of the rectangle
  #
  # Overrides `Object#inspect` to more closely mimic MacRuby `Boxed#inspect`.
  #
  # @return [String]
  def inspect
    "#<CGRect origin=#{self.origin.inspect} size=#{self.size.inspect}>"
  end

end


require 'uri'

##
# `accessibility-core` extensions to the `URI` family of classes
class URI::Generic
  ##
  # Returns the receiver (since the receiver is already a `URI` object)
  #
  # @return [URI::Generic]
  def to_url
    self
  end
end

##
# `accessibility-core` extensions to the `String` class
class String
  ##
  # Parse the receiver into a `URI` object
  #
  # @return [URI::Generic]
  def to_url
    URI.parse self
  end
end

##
# `accessibility-core` extensions to the `Object` class
class Object
  # (see NSObject#to_ruby)
  def to_ruby
    self
  end

  ##
  # Whether or not the `outer` rect completely encloses the `inner` rect
  #
  # @param outer [CGRect,#to_rect]
  # @param inner [CGRect,#to_rect]
  # @return [Boolean]
  def NSContainsRect outer, inner
    outer.to_rect.contains? inner
  end
end

##
# `accessibility-core` extensions to the `Array` class
class Array
  # (see NSArray#to_ruby)
  def to_ruby
    map do |obj| obj.to_ruby end
  end
end


##
# `accessibility-core` extensions to the `Range` class
class Range

  ##
  # Returns a new Range instance which has negative values in
  # the receiver expanded relative to `max`
  #
  # @example
  #
  #  (1..10).relative_to(10)   # => (1..10)
  #  (-3..-1).relative_to(10)  # => (7..9)
  #
  # @param max [Fixnum]
  # @return [Range]
  def relative_to max
    beg = adjust_index self.begin, max
    len = adjust_index self.end, max
    len -= 1 if exclude_end?
    beg..len
  end


  private

  def adjust_index val, max
    if val >= max
      exclude_end? ? max : max - 1
    elsif val < 0
      max + val
    else
      val
    end
  end

end


require 'accessibility/bridge/common'
require 'accessibility/bridge/bridge.bundle'
