task :default => :test

require 'rake/clean'
CLEAN.include '*.plist', '*.gch'

desc 'Run the Clang static analyzer'
task :analyze do
  sh "clang --analyze ext/accessibility/bridge/bridge.c"
  sh "clang --analyze ext/accessibility/extras/extras.c"
  sh "clang --analyze ext/accessibility/highlighter/highlighter.c"
end

desc 'Startup an IRb console with accessibility-core loaded'
task :console => [:compile] do
  sh 'irb -Ilib -raccessibility/bridge -raccessibility/extras -raccessibility/highlighter'
end

require 'rake/testtask'
Rake::TestTask.new do |t|
  t.libs << '.'
  t.pattern = 'test/*_test.rb'
end
task :test => :compile


# Gem stuff

require 'rubygems/package_task'
SPEC = Gem::Specification.load('accessibility_bridge.gemspec')

Gem::PackageTask.new(SPEC) { }

desc 'Build and install gem (not including deps)'
task :install => :gem do
  require 'rubygems/installer'
  Gem::Installer.new("pkg/#{SPEC.file_name}").install
end


# C extensions!

require 'rake/extensiontask'

Rake::ExtensionTask.new('bridge', SPEC) do |ext|
  ext.ext_dir = 'ext/accessibility/bridge'
  ext.lib_dir = 'lib/accessibility'
end

Rake::ExtensionTask.new('extras', SPEC) do |ext|
  ext.ext_dir = 'ext/accessibility/extras'
  ext.lib_dir = 'lib/accessibility'
end

Rake::ExtensionTask.new('highlighter', SPEC) do |ext|
  ext.ext_dir = 'ext/accessibility/highlighter'
  ext.lib_dir = 'lib/accessibility'
end
