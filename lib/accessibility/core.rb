require 'accessibility/core/version'

if RUBY_ENGINE == 'macruby'

  framework 'Cocoa'

  # A workaround that guarantees that `CGPoint` is defined
  MOUNTAIN_LION_APPKIT_VERSION = 1187
  if NSAppKitVersionNumber >= MOUNTAIN_LION_APPKIT_VERSION
    framework '/System/Library/Frameworks/CoreGraphics.framework'
  end

  ##
  # accessibility-core extensions for `NSURL`
  class NSString
    ##
    # Create an NSURL using the receiver as the initialization string.
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


else

  require 'uri' # TODO figure out what to do with NSURL <-> URI stuff

  ##
  # A structure that contains a point in a two-dimensional coordinate system
  class CGPoint < Struct.new(:x, :y)

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
  class CGSize < Struct.new(:width, :height)

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
  class CGRect < Struct.new(:origin, :size)

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

end

class CGPoint
  ##
  # Returns the receiver, since the receiver is already a {CGPoint}
  #
  # @return [CGPoint]
  def to_point
    self
  end
end

class CGSize
  ##
  # Returns the receiver, since the receiver is already a {CGSize}
  #
  # @return [CGSize]
  def to_size
    self
  end
end

class CGRect
  ##
  # Returns the receiver, since the receiver is already a {CGRect}
  #
  # @return [CGRect]
  def to_rect
    self
  end
end

##
# accessibility-core extensions to `Range`
class Range
  ##
  # Returns the receiver, since the receiver is already a {Range}
  #
  # @return [Range]
  def to_range
    self
  end
end

##
# accessibility-core extensions to `Array`
class Array
  ##
  # Coerce the first two elements of the receiver into a {CGPoint}
  #
  # @return [CGPoint]
  def to_point
    CGPoint.new self[0], self[1]
  end

  ##
  # Coerce the first two elements of the receiver into a {CGSize}
  #
  # @return [CGSize]
  def to_size
    CGSize.new self[0], self[1]
  end

  ##
  # Coerce the first four elements of the receiver into a {CGRect}
  #
  # @return [CGRect]
  def to_rect
    CGRect.new CGPoint.new(self[0], self[1]), CGSize.new(self[2], self[3])
  end
end

# TODO
# NSURL -> URI
# NSDate -> Time

require 'caccessibility' # the bundle
