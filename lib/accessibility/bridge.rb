require 'accessibility/bridge/version'

if defined? MACRUBY_REVISION

  ##
  # Whether or not we are running on MacRuby
  def on_macruby?
    true
  end

  framework 'Cocoa'

  # A workaround that guarantees that `CGPoint` is defined
  unless defined? MOUNTAIN_LION_APPKIT_VERSION
    MOUNTAIN_LION_APPKIT_VERSION = 1187
  end

  if NSAppKitVersionNumber >= MOUNTAIN_LION_APPKIT_VERSION
    framework '/System/Library/Frameworks/CoreGraphics.framework'
  end

  require 'accessibility/bridge/macruby'

else

  def on_macruby?
    false
  end

  require 'accessibility/bridge/mri'

end

