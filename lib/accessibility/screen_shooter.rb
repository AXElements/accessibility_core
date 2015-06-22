require 'accessibility/bridge'
require 'accessibility/screen_shooter/screen_shooter.bundle'

module ScreenShooter

  def shoot rect = CGRect.new(CGPoint.new(0, 0), CGSize.new(-1, -1)),
            path = '~/Downloads'

    path = File.expand_path path.to_s
    path = "#{path}/AXElements-ScreenShot-#{Time.now.strftime '%Y%m%d%H%M%S'}.png"

    raise 'Failed to save screenshot' unless screenshot rect, path

    path
  end

end
