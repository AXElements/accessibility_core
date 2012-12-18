task :default => :test

require 'rake/clean'
CLEAN.include '*.plist', '*.gch'

desc 'Run the Clang static analyzer'
task :analyze do
  sh "clang --analyze ext/accessibility/core/core.c"
end

desc 'Startup an IRb console with accessibility-core loaded'
task :console => [:compile] do
  sh 'irb -Ilib -raccessibility/core'
end

desc 'Open the fixture app'
task :run_fixture => :fixture do
  sh 'open test/fixture/Release/AXElementsTester.app'
end

desc 'Build the test fixture'
task :fixture do
  sh 'cd test/AXElementsTester && xcodebuild'
end

desc 'Remove the built fixture app'
task :clobber_fixture do
  $stdout.puts 'rm -rf test/fixture'
  rm_rf 'test/fixture'
end
task :clobber => :clobber_fixture

require 'rake/testtask'
Rake::TestTask.new do |t|
  t.libs << '.'
  t.pattern = 'test/*_test.rb'
end
task :test => [:compile, :fixture]


# Gem stuff

require 'rubygems/package_task'
SPEC = Gem::Specification.load('accessibility_core.gemspec')

Gem::PackageTask.new(SPEC) { }

desc 'Build and install gem (not including deps)'
task :install => :gem do
  require 'rubygems/installer'
  Gem::Installer.new("pkg/#{SPEC.file_name}").install
end


# C extensions!

require 'rake/extensiontask'

Rake::ExtensionTask.new('core', SPEC) do |ext|
  ext.ext_dir = 'ext/accessibility/core'
  ext.lib_dir = 'lib/accessibility/core'
end

Rake::ExtensionTask.new('highlighter', SPEC) do |ext|
  ext.ext_dir = 'ext/accessibility/highlighter'
  ext.lib_dir = 'lib/accessibility/core'
end

# Rake::ExtensionTask.new('running_application', SPEC) do |ext|
#   ext.ext_dir = 'ext/accessibility/running_application'
#   ext.lib_dir = 'lib/accessibility/core'
# end
