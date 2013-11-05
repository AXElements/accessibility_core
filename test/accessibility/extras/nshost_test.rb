require 'test/helper'
require 'accessibility/extras'

class NSHostTest < MiniTest::Unit::TestCase
#  try_to_parallelize!

  def host
    NSHost.currentHost
  end

  def test_names
    names = host.names
    assert_kind_of Array, names
    assert_kind_of String, names.first
    refute_empty names.first

    unless on_macruby? # lol?
      assert_equal `macruby -e 'puts NSHost.currentHost.names.first'`.chomp, host.names.first
    end
  end

  def test_addresses
    addrs = host.addresses
    assert_kind_of Array, addrs
    assert_kind_of String, addrs.first
    assert_includes addrs, '127.0.0.1', 'Is IPv4 disabled?'
  end

  def test_localized_name
    name = host.localizedName
    assert_kind_of String, name
    refute_empty name

    unless on_macruby? # lol?
      assert_equal `macruby -e 'puts NSHost.currentHost.localizedName'`.chomp, host.localizedName
    end
  end

end
