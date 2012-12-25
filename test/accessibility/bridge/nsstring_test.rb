require 'test/helper'
require 'accessibility/bridge'

if on_macruby?
class NSStringTest < MiniTest::Unit::TestCase

  def test_to_url
    site = 'http://marketcircle.com/'
    url  = site.to_url
    refute_nil url
    assert_equal NSURL.URLWithString(site), url

    file = 'file://localhost/Applications/Calculator.app/'
    url  = file.to_url
    refute_nil url
    assert_equal NSURL.fileURLWithPath('/Applications/Calculator.app/'), url

    void = 'not a url at all'
    assert_nil void.to_url
  end

end
end
