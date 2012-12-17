require 'test/helper'

class StringTest < MiniTest::Unit::TestCase

  if on_macruby?
    def test_to_url
      site = 'http://marketcircle.com/'
      url  = site.to_url
      refute_nil url
      assert_equal NSURL.URLWithString(site), url

      file = 'file://localhost/Applications/Calculator.app/'
      url  = file.to_url
      refute_nil url
      assert_equal NSURL.fileURLWithPath('/Applications/Calculator.app/'), url

      assert_nil 'not a url at all'.to_url
    end

  else

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

end
