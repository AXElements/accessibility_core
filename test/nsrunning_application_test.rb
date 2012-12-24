require 'test/helper'
require 'accessibility/extras'

class NSRunningApplicationTest < MiniTest::Unit::TestCase

  def app
    @app ||= NSRunningApplication.runningApplicationWithProcessIdentifier(Process.pid)
  end

  def terminals
    NSRunningApplication.runningApplicationsWithBundleIdentifier('com.apple.Terminal')
  end

  def test_running_app_with_pid
    assert_nil NSRunningApplication.runningApplicationWithProcessIdentifier(0)

    assert_kind_of NSRunningApplication, app
    assert_equal RUBY_ENGINE, app.localizedName
  end

  def test_running_apps_with_bundle_id
    assert_kind_of Array, terminals
    assert_kind_of NSRunningApplication, terminals.first
  end

  # what does this even mean for a regular ruby script?
  def test_running_app_current_app
    assert_equal app, NSRunningApplication.currentApplication
  end

  def test_frontmost_app_is_active?
    terminals.first.activateWithOptions NSRunningApplication::NSApplicationActivateIgnoringOtherApps
    spin 0.2
    assert_equal NSWorkspace.menuBarOwningApplication,
     terminals.first
  end

  def test_activate_with_opts
    assert terminals.first.
     activateWithOptions(NSRunningApplication::NSApplicationActivateIgnoringOtherApps)
  end

  def test_hide_unhide
    terminal = terminals.first
    spin 0.2
    terminal.hide
    spin 0.2
    assert terminal.hidden?
    terminal.unhide
    spin 0.2
    refute terminal.hidden?
  ensure
    terminals.first.activateWithOptions NSRunningApplication::NSApplicationActivateIgnoringOtherApps
    spin 0.2
  end

  def test_localized_name
    assert_equal RUBY_ENGINE, app.localizedName
    assert_equal 'Terminal', terminals.first.localizedName # might fail in different locales?
  end

  def test_bundle_id
    assert_nil app.bundleIdentifier
    assert_equal 'com.apple.Terminal', terminals.first.bundleIdentifier
  end

  def test_bundle_url
    assert_nil app.bundleURL
    assert_equal '/Applications/Utilities/Terminal.app/', terminals.first.bundleURL.path
  end

  def test_executable_arch
    assert_kind_of Fixnum, app.executableArchitecture
  end

  def test_executable_url
    assert_equal "#{RbConfig::CONFIG['bindir']}/#{RUBY_ENGINE}", app.executableURL.path
  end

  def test_launch_date
    date = terminals.first.launchDate
    assert_kind_of Time, date
    assert Time.now > date
  end

  def test_finished_launching
    refute app.finishedLaunching?
    assert terminals.first.finishedLaunching?
  end

  def test_process_id
    assert_equal Process.pid, app.processIdentifier
  end

  def test_owns_menu_bar
    refute app.ownsMenuBar?
    assert terminals.first.ownsMenuBar?

    refute app.ownsMenuBar?
    assert terminals.first.ownsMenuBar?
  end

  def test_terminated?
    refute app.terminated?
    # @todo test some other stuff...launch app and quit it?
  end

end
