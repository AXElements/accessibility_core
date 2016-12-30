require 'test/helper'
require 'accessibility/extras'

class NSProcessInfoTest < Minitest::Test

  def pinfo
    NSProcessInfo.processInfo
  end

  def test_os_version_string
    str = pinfo.operatingSystemVersionString
    assert_kind_of String, str
    assert_match(/Version 10/, str)
    assert_match(/\(Build /, str)
  end

  def test_system_uptime
    assert_kind_of Float, pinfo.systemUptime
    assert pinfo.systemUptime > 0
  end

  def test_p_count
    assert_equal `sysctl -n hw.ncpu`.chomp.to_i, pinfo.processorCount
  end

  def test_active_p_count
    assert_equal `sysctl -n hw.activecpu`.chomp.to_i, pinfo.activeProcessorCount
  end

  def test_physical_memory
    assert_equal `sysctl -n hw.memsize`.chomp.to_i, pinfo.physicalMemory
  end

end
