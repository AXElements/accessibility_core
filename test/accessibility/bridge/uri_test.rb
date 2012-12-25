require 'test/helper'

unless on_macruby?
class TestURIExtensions < MiniTest::Unit::TestCase

  def test_to_url_returns_self
    url = URI.parse 'http://macruby.org'
    assert_same url, url.to_url

    url = URI.parse 'ftp://herp.com'
    assert_same url, url.to_url
  end

end
end
