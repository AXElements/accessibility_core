task :default => :test

# @todo restore the clang analyze task, don't forget to add rubyhdrdir

desc 'Startup an IRb console with accessibility-core loaded'
task :console => 'compile:core' do
  sh 'irb -Ilib -raccessibility/core'
end

desc 'Startup an IRb console with everything loaded'
task :console_complete => :compile do
  sh 'irb -Ilib -raccessibility/bridge -raccessibility/core -raccessibility/extras -raccessibility/highlighter'
end

# Gem stuff

require 'rubygems/package_task'
SPEC = Gem::Specification.load('accessibility_core.gemspec')

Gem::PackageTask.new(SPEC) { }

desc 'Build and install gem (not including deps)'
task :install => :gem do
  require 'rubygems/installer'
  Gem::Installer.new("pkg/#{SPEC.file_name}").install
end

