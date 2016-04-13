require 'rbconfig'
base_sdk = `xcrun --show-sdk-path`.chomp
match_data = /MacOSX(\d+)\.(\d+)\.sdk/.match(base_sdk)
minor_version = match_data[2].to_i
# 10.11 is darwin 15
darwin_version = 15 + (minor_version - 11)
RbConfig::CONFIG['arch'] = "universal-darwin#{darwin_version}"
RbConfig::CONFIG['rubyarchhdrdir'] = RbConfig::expand('$(rubyhdrdir)/$(arch)')
require 'mkmf'