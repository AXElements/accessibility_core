require 'test/helper'
require 'accessibility/bridge'

class TestNSData < Minitest::Test

  def path
    '/Applications/Calendar.app/Contents/Info.plist'
  end

  def plist
    NSData.dataWithContentsOfURL URI.parse "file://#{path}"
  end

  def test_data
    assert_respond_to NSData, :data
    assert_equal NSData.data, NSData.data
  end

  def test_length
    assert_equal 0, NSData.data.length
    assert plist.length > 0
  end

  def test_to_str
    str = plist.to_str
    assert_equal Encoding::UTF_8, str.encoding
    assert_equal File.read(path), plist.to_str
  end

  def test_contents_of_url
    refute_nil plist
  end

  def test_write_to_file
    derp_path = "/tmp/derp-#{Time.now}.plist"
    plist.writeToFile derp_path, atomically: true
    assert File.exist? derp_path
    assert_equal plist.to_str, File.read(derp_path)
  end

end

