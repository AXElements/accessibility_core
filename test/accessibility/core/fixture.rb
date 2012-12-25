# We want to launch the test app and make sure it responds to
# accessibility queries, but that is difficult to know at what
# point it will start to respond, so we just sleep
#
# We also want to make it registers it's at_exit hook before
# minitest does!
APP_BUNDLE_PATH       = File.expand_path './test/fixture/Release/AXElementsTester.app'
APP_BUNDLE_IDENTIFIER = 'com.marketcircle.AXElementsTester'

`open #{APP_BUNDLE_PATH}`
sleep 3

at_exit do
  `killall AXElementsTester`
end


require 'test/helper'
require 'accessibility/core'


class MiniTest::Unit::TestCase

  PID = if on_macruby?
          NSWorkspace.sharedWorkspace.runningApplications.find do |app|
            app.bundleIdentifier == APP_BUNDLE_IDENTIFIER
          end.processIdentifier
        else
          # a bit fragile, need to find a better solution
          `ps aux | grep AXElementsTester`.chomp.split("\n").last.split(' ').at(1).to_i
        end

  # if this basic API doesn't work we might as well as give up right away
  APP = Accessibility::Element.application_for PID

end
