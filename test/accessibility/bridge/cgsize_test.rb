require 'test/helper'
require 'accessibility/bridge'

class CGSizeTest < MiniTest::Unit::TestCase

  def test_attributes
    s = CGSize.new
    assert_respond_to s, :width
    assert_respond_to s, :height
  end

  def test_to_a
    s = CGSize.new 1, 2
    assert_equal [1, 2], s.to_a
  end

  def test_to_size
    s = CGSize.new
    assert_same s, s.to_size

    s = CGSize.new 4, 2
    assert_same s, s.to_size
  end

  def test_initialize
    s = CGSize.new
    assert_equal 0, s.width
    assert_equal 0, s.height

    w, h = rand_nums 2
    s = CGSize.new w, h
    assert_equal w, s.width
    assert_equal h, s.height

    w, h = rand_floats 2
    s = CGSize.new w, h
    assert_equal w, s.width
    assert_equal h, s.height
  end

  def test_inspect
    assert_match /Size width=1.0 height=2.0>/, CGSize.new(1, 2).inspect
    assert_match /Size width=3.0 height=5.0>/, CGSize.new(3, 5).inspect
  end

end
