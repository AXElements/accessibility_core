require 'test/helper'
require 'accessibility/extras'

class ObjectTest < Minitest::Test

  def test_load_plist
    input = File.read '/System/Library/Accessibility/AccessibilityDefinitions.plist'
    plist = load_plist input
    assert_kind_of Hash, plist
    assert plist.has_key? 'types'
  end

end

