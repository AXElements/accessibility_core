require 'test/helper'
require 'accessibility/bridge'

class ObjectTest < Minitest::Test

  def test_to_ruby
    assert_same Object, Object.to_ruby
    assert_same 10, 10.to_ruby
  end

  # running this on MacRuby gives expected behaviour,
  # then run on MRI to check that behaviour matches
  def test_NSContainsRect
    rect = [100, 100, 100, 100]

    # test rect of exactly the same size
    assert NSContainsRect(rect, [100,100,100,100])

    # rect to left
    refute NSContainsRect(rect, [  0,100,100,100])
    # rect above
    refute NSContainsRect(rect, [100,  0,100,100])
    # rect very thin, but still inside
    assert NSContainsRect(rect, [100,100,  1, 10])
    # rect very short, but still inside
    assert NSContainsRect(rect, [100,100,100,  1])
    # rect in the middle
    assert NSContainsRect(rect, [111,111, 11, 11])

    # rects mostly inside, but out too much on one side
    refute NSContainsRect(rect, [100,100,110, 90]) # right
    refute NSContainsRect(rect, [190,100, 11, 10]) # right
    refute NSContainsRect(rect, [100,190, 10, 11]) # bottom
    refute NSContainsRect(rect, [100,100, 10,110]) # bottom
    refute NSContainsRect(rect, [ 90,100,100, 20]) # left
    refute NSContainsRect(rect, [100, 90, 10,100]) # top

    # rects completely outside
    refute NSContainsRect(rect, [256,256,  1,  1])
    refute NSContainsRect(rect, [  0,  0,  1,  1])
    refute NSContainsRect(rect, [ 50, 50, 50, 50])

    # zero width/height seems to be a special case
    # which is easy to test in C, much more expensive in Ruby
    refute NSContainsRect(rect, [100,100,  0,  0])
    refute NSContainsRect(rect, [100,100,  0,  1])
    refute NSContainsRect(rect, [100,100,  1,  0])
    refute NSContainsRect(rect, [101,101,  0,  0])
    refute NSContainsRect(rect, [101,101,  0,  1])
    refute NSContainsRect(rect, [101,101,  1,  0])
  end

  def test_spin
    assert_respond_to Object.new, :spin
    assert_equal(-1, Object.method(:spin).arity)

    start = Time.now
    Object.new.spin
    assert Time.now - start < 0.01

    start = Time.now
    Object.new.spin 0.2
    assert Time.now - start > 0.2
  end

  def test_description
    [
      self,
      rand(1000),
      Time.now,
      'a string'
    ].each do |obj|
      assert_equal obj.inspect, obj.description
    end
  end

  # we may have other tests for extensions to the Object class

end
