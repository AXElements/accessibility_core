require 'test/helper'
require 'accessibility/extras'

class NSBundleTest < MiniTest::Unit::TestCase

  def cocoa
    'file:///System/Library/Frameworks/Cocoa.framework'.to_url
  end

  def bundle
    NSBundle.bundleWithURL cocoa
  end

  def test_bundle_with_url
    assert_kind_of NSBundle, bundle
  end

  def test_info_dictionary
    2.times do
      info = bundle.infoDictionary
      assert_kind_of Hash, info
      refute_empty info
    end
  end

  def test_object_for_info_dict_key
    2.times do
      obj = bundle.objectForInfoDictionaryKey 'CFBundleShortVersionString'
      assert_kind_of String, obj
    end
  end

end
