framework 'Cocoa'

# A workaround that guarantees that `CGPoint` is defined
MOUNTAIN_LION_APPKIT_VERSION = 1187
if NSAppKitVersionNumber >= MOUNTAIN_LION_APPKIT_VERSION
  framework '/System/Library/Frameworks/CoreGraphics.framework'
end

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
