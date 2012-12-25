require 'rake/testtask'

['bridge','core','extras','highlighter'].each do |test|
  namespace :test do
    Rake::TestTask.new test do |t|
      t.libs << '.'
      t.pattern = "test/accessibility/#{test}/*_test.rb"
    end
    task test => "compile:#{test}"
  end
  task :test => "test:#{test}"
end
task 'test:core' => :fixture


desc 'Build the test fixture'
task :fixture do
  sh 'cd test/AXElementsTester && xcodebuild'
end

desc 'Open the fixture app'
task :run_fixture => :fixture do
  sh 'open test/fixture/Release/AXElementsTester.app'
end

desc 'Remove the built fixture app'
task :clobber_fixture do
  $stdout.puts 'rm -rf test/fixture'
  rm_rf 'test/fixture'
end
task :clobber => :clobber_fixture

