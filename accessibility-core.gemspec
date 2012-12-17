require './lib/accessibility/core/version'

Gem::Specification.new do |s|
  s.name     = 'accessibility-core'
  s.version  = Accessibility::Core::VERSION

  s.summary     = 'A library for building automation tools on OS X'
  s.description = <<-EOS
accessibility-core is a wrapper around the OS X Accessibility framework.

Originally extracted from the AXElements project.
  EOS

  s.authors     = ['Mark Rada']
  s.email       = 'markrada26@gmail.com'
  s.homepage    = 'http://github.com/ferrous26/accessibility'
  s.licenses    = ['BSD 3-clause']
  s.has_rdoc    = 'yard'

  s.extensions  = [
                   'ext/accessibility/core/extconf.rb',
                   'ext/accessibility/highlighter/extconf.rb',
                   #'ext/accessibility/running_application/extconf.rb'
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
