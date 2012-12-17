##
# A structure that contains a point in a two-dimensional coordinate system
CGPoint = Struct.new(:x, :y) do

  # @param x [Number]
  # @param y [Number]
  def initialize x = 0.0, y = 0.0
    super
  end

  # @!attribute [rw] x
  #   The `x` co-ordinate of the screen point
  #   @return [Number]

  # @!attribute [rw] y
  #   The `y` co-ordinate of the screen point
  #   @return [Number]

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
CGSize = Struct.new(:width, :height) do

  # @param width [Number]
  # @param height [Number]
  def initialize width = 0.0, height = 0.0
    super
  end

  # @!attribute [rw] width
  #   The `width` of the box
  #   @return [Number]

  # @!attribute [rw] height
  #   The `heighth` of the box
  #   @return [Number]

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
CGRect = Struct.new(:origin, :size) do

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
