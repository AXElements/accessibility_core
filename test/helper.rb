require 'minitest/autorun'
require 'minitest/pride'
require 'accessibility/bridge'

class MiniTest::Unit::TestCase

  if respond_to? :parallelize_me!
    class << self
      alias_method :try_to_parallelize!, :parallelize_me!
    end
  else
    def self.try_to_parallelize!
    end
  end

  def rand_nums count, range = 1_000
    Array.new count do
      rand range
    end
  end

  def rand_floats count, range = 1_000.0
    Array.new count do
      rand * range
    end
  end

end
