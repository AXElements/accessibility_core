require 'test/helper'
require 'accessibility/bridge'

class StringTest < MiniTest::Unit::TestCase

  def test_to_url
    site = 'http://marketcircle.com/'
    url  = site.to_url
    refute_nil url
    assert_equal URI.parse(site), url

    # Ruby's URI class does not handle the file scheme :(
    # @todo propose the change upstream at some point?

    assert_raises(URI::InvalidURIError) { 'not a url at all'.to_url }
  end

end
