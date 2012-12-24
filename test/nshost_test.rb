require 'test/helper'
require 'accessibility/extras'

class NSHostTest < MiniTest::Unit::TestCase

  def host
    NSHost.currentHost
  end

  def test_names
    assert_equal `hostname`.chomp, host.names.first
  end

  def test_addresses
    addrs = host.addresses
    assert_kind_of Array, addrs
    assert_kind_of String, addrs.first
    assert_includes addrs, '127.0.0.1', 'Is IPv4 disabled?'
  end

  def test_localized_name
    assert_equal `hostname -s`.chomp, host.localizedName, 'Do you have a custom bonjour name?'
  end

end
