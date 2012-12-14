require 'test/helper'

if on_macruby?
class TestNSURL < MiniTest::Unit::TestCase

  def test_to_url_returns_self
    url = NSURL.URLWithString 'http://macruby.org'
    assert_same url, url.to_url
  end

end
end
