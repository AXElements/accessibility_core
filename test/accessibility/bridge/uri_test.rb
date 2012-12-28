require 'test/helper'
require 'accessibility/bridge'

class TestURIExtensions < MiniTest::Unit::TestCase

  if on_macruby?
    def parse url
      NSURL.URLWithString url
    end
  else
    def parse url
      URI.parse url
    end
  end

  def test_to_url_returns_self
    url = parse 'http://macruby.org'
    assert_same url, url.to_url

    url = parse 'ftp://herp.com'
    assert_same url, url.to_url
  end

  def test_last_path_component
    url = parse 'https://macruby.macosforge.org/files/nightlies/macruby_nightly-latest.pkg'
    assert_equal 'macruby_nightly-latest.pkg', url.lastPathComponent

    url = parse 'file:///localhost/Users/mrada/Desktop/'
    assert_equal 'Desktop', url.lastPathComponent
  end

  def test_path_extension
    url = parse 'https://macruby.macosforge.org/files/nightlies/macruby_nightly-latest.pkg'
    assert_equal 'pkg', url.pathExtension

    url = parse 'file:///localhost/Users/mrada/Desktop'
    assert_equal '', url.pathExtension

    url = parse 'file:///localhost/Users/mrada/Desktop/'
    assert_equal '', url.pathExtension
  end

end
