require 'test/helper'
require 'accessibility/extras'

class NSWorkspaceTest < Minitest::Test

  def shared
    NSWorkspace.sharedWorkspace
  end

  def test_running_applications
    apps = shared.runningApplications
    assert_kind_of Array, apps
    assert_kind_of NSRunningApplication, apps.first
  end

  def test_frontmost_app
    assert_equal 'com.apple.Terminal', shared.frontmostApplication.bundleIdentifier
  end

  def test_menu_bar_owner
    assert_equal 'com.apple.Terminal', shared.menuBarOwningApplication.bundleIdentifier
  end

  def test_launch_app_and_terminate_it
    id = 'com.apple.Grab'
    assert shared.launchAppWithBundleIdentifier( id,
                                        options: NSWorkspace::NSWorkspaceLaunchAsync,
                 additionalEventParamDescriptor: nil,
                               launchIdentifier: nil)
    spin 1
    app = NSRunningApplication.runningApplicationsWithBundleIdentifier(id).first
    app.terminate
    spin 0.2
    assert app.terminated?
  end

end
