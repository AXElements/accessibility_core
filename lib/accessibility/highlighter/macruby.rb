##
# A screen highlighter for debugging
#
# When you initialize a highligter object it will highlight the given
# bounds on the screen.
#
# Highligter objects can have their colour configured at initialization,
# and can also have a timeout to automatically stop displaying.
#
# Options for setting up a highlighter are document in
# {Accessibility::Highlighter#initialize}.
#
# @example
#
#   h = Accessibility::Highlighter.new [100,100,100,100]
#   # when you are done...
#   h.stop
#
class Accessibility::Highlighter < NSWindow

  # @param bounds [CGRect,#to_rect]
  # @param opts [Hash]
  # @option opts [Number] :timeout
  # @option opts [NSColor] :colour (NSColor.magentaColor)
  def initialize bounds, opts = {}
    color = opts[:colour] || opts[:color] || NSColor.magentaColor

    bounds = bounds.to_rect
    bounds.flip! # we assume the rect is in the other co-ordinate system

    initWithContentRect bounds,
             styleMask: NSBorderlessWindowMask,
               backing: NSBackingStoreBuffered,
                 defer: true
    setOpaque false
    setAlphaValue 0.20
    setLevel NSStatusWindowLevel
    setBackgroundColor color
    setIgnoresMouseEvents true
    setFrame bounds, display: false
    makeKeyAndOrderFront NSApp

    if opts.has_key? :timeout
      Dispatch::Queue.concurrent.after opts[:timeout] do
        self.stop
      end
    end
  end

  ##
  # Tell the highlighter to stop displaying.
  #
  # @return [self]
  def stop
    close
  end

  ##
  # Returns the color for the highlighter
  #
  # @return [NSColor]
  def color
    backgroundColor
  end
  alias_method :colour, :color

  ##
  # Return whether or not the highlighter is currently drawn on screen
  def visible?
    super
  end

  ##
  # Return the rectangle on screen which the highlighter is occupying
  #
  # @return [CGRect]
  def frame
    super
  end

end


# Initialize the shared application so that windows can be created
NSApplication.sharedApplication
