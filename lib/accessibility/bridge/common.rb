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

  ##
  # Treats the rect as belonging to one co-ordinate system and then
  # converts it to the other system
  #
  # This is useful because accessibility API's expect to work with
  # the flipped co-ordinate system (origin in top left), but AppKit
  # prefers to use the cartesian co-ordinate system (origin in bottom
  # left).
  #
  # @return [CGRect]
  def flip!
    frame         = ::NSScreen.screens.first.frame
    screen_height = frame.origin.y + frame.size.height
    self_max_y    = self.origin.y + self.size.height
    origin.y      = screen_height - self_max_y
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
