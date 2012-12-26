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
  # Whether or not the receiver completely encloses the `inner` rect
  #
  # On MacRuby this is equivalent to calling `NSContainsRect()`.
  #
  # @param inner [CGRect,#to_rect]
  # @return [Boolean]
  def contains? inner
    ox = origin.x; oy = origin.y; ow = size.width; oh = size.height

    inner = inner.to_rect
    ix    = inner.origin.x;   iy = inner.origin.y
    iw    = inner.size.width; ih = inner.size.height

    if iw.zero? || ih.zero?
      false
    else
      (ox <= ix)               && (oy <= iy)               &&
      ((ox + ow) >= (ix + iw)) && ((oy + oh) >= (iy + ih))
    end
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
