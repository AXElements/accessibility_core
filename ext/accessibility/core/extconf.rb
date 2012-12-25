require 'mkmf'

$CFLAGS << ' -std=c99 -Wall -Werror -pedantic -ObjC'
$LIBS   << ' -framework CoreFoundation -framework ApplicationServices -framework Cocoa'
$LIBS   << ' -framework CoreGraphics' unless `sw_vers -productVersion`.to_f == 10.7

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

create_makefile 'accessibility/core'

# modify the bugger so we can depend on bridge.h properly
makefile = File.read 'Makefile'
makefile.gsub! '$(DLLIB): $(OBJS) Makefile', '$(DLLIB): $(OBJS) Makefile ../bridge/bridge.o'
makefile.gsub! '$(Q) $(LDSHARED) -o $@', '$(Q) $(LDSHARED) -o $@ ../bridge/bridge.o'
File.open('Makefile', 'w') { |f| f.write makefile }
