require 'test/helper'
require 'accessibility/extras'

class NSBundleTest < MiniTest::Unit::TestCase

  def cocoa
    'file:///System/Library/Frameworks/Cocoa.framework'.to_url
  end

  def test_bundle_with_url
    bundle = NSBundle.bundleWithURL cocoa
    assert_kind_of NSBundle, bundle
  end

  def test_info_dictionary
    bundle = NSBundle.bundleWithURL cocoa
    info   = bundle.infoDictionary
    assert_kind_of Hash, info
    refute_empty info
  end

end
