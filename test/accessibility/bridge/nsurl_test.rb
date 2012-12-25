require 'test/helper'
require 'accessibility/bridge'

if on_macruby?
class TestNSURL < MiniTest::Unit::TestCase

  def test_to_url_returns_self
    url = NSURL.URLWithString 'http://macruby.org'
    assert_same url, url.to_url
  end

  def test_to_s
    s = 'http://marketcircle.com'
    assert_equal s, NSURL.URLWithString(s).to_s
  end

end
end
