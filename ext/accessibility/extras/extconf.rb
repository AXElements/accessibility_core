require 'mkmf'

$CFLAGS << ' -std=c99 -Wall -Werror -pedantic -ObjC'
$LIBS   << ' -framework CoreFoundation -framework Cocoa -framework IOKit'

if RUBY_ENGINE == 'macruby'
  $CFLAGS << ' -fobjc-gc'
else
  unless RbConfig::CONFIG["CC"].match /clang/
    clang = `which clang`.chomp
    if clang.empty?
      raise "Clang not installed. Cannot build C extension"
    else
      RbConfig::MAKEFILE_CONFIG["CC"]  = clang
      RbConfig::MAKEFILE_CONFIG["CXX"] = clang
    end
  end
  $CFLAGS << ' -DNOT_MACRUBY'
end

create_makefile 'accessibility/extras'
