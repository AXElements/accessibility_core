class CGRect
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
