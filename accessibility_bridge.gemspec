require './lib/accessibility/bridge/version'

Gem::Specification.new do |s|
  s.name     = 'accessibility_bridge'
  s.version  = Accessibility::Bridge::VERSION

  s.summary     = 'A library for bridging into Cocoa on OSX '
  s.description = <<-EOS
accessibility_bridge is a wrapper around various bits of Cocoa
so that the various accessibility projects can run on CRuby.

Originally extracted from the AXElements project.
  EOS

  s.authors     = ['Mark Rada']
  s.email       = 'markrada26@gmail.com'
  s.homepage    = 'http://github.com/AXElements/accessibility_bridge'
  s.licenses    = ['BSD 3-clause']
  s.has_rdoc    = 'yard'

  s.extensions  = [
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

  s.add_development_dependency 'yard', '~> 0.8.3'
  s.add_development_dependency 'kramdown', '~> 0.14.1'
  s.add_development_dependency 'rake-compiler', '~> 0.8.1'
end
