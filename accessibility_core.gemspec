require './lib/accessibility/core/version'

Gem::Specification.new do |s|
  s.name     = 'accessibility_core'
  s.version  = Accessibility::Core::VERSION

  s.summary     = 'A library for building automation tools on OS X'
  s.description = <<-EOS
accessibility_core is a wrapper around the OS X Accessibility framework.

Some other extras that are intended to help build higher level abstractions
have also been included. They are primarily wrappers around misc bits
of Cocoa, but include some speciality modules.

Originally extracted from the AXElements project.
  EOS

  s.authors     = ['Mark Rada']
  s.email       = 'markrada26@gmail.com'
  s.homepage    = 'http://github.com/AXElements/accessibility_core'
  s.licenses    = ['BSD 3-clause']
  s.has_rdoc    = 'yard'

  s.extensions  = [
                   'ext/accessibility/core/extconf.rb',
                   'ext/accessibility/bridge/extconf.rb',
                   'ext/accessibility/extras/extconf.rb',
                   'ext/accessibility/highlighter/extconf.rb'
                  ]
  s.files       = Dir.glob('lib/**/*.rb') +
                  Dir.glob('ext/**/*.{c,h,rb}') +
                  [
                   'Rakefile',
                   'README.markdown',
                   'History.markdown',
                   '.yardopts'
                  ]
  s.test_files  = Dir.glob('test/**/test_*.rb') + [ 'test/helper.rb' ]
end
