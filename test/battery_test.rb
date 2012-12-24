require 'test/helper'
require 'accessibility/extras'

class BatteryTest < MiniTest::Unit::TestCase

  def states
    [:not_installed, :charged, :charging, :discharging]
  end

  def test_state
    assert_includes states, Battery.state
  end

  def test_level
    assert_equal Battery.charge_level, Battery.level
    assert_kind_of Float, Battery.level
    assert Battery.level > 0 || Battery.level == -1.0
  end

  def test_time_to_discharged_or_charged
    assert_kind_of Fixnum, Battery.time_to_discharged
    assert_kind_of Fixnum, Battery.time_to_empty
    assert_kind_of Fixnum, Battery.time_to_charged
    assert_kind_of Fixnum, Battery.time_to_full_charge

    assert_equal Battery.time_to_discharged, Battery.time_to_empty
    assert_equal Battery.time_to_charged, Battery.time_to_full_charge

    assert Battery.time_to_empty.zero? || Battery.time_to_full_charge.zero?
    assert Battery.time_to_empty > 0 || Battery.time_to_full_charge > 0
  end

end
