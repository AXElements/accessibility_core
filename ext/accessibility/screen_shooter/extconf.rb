require 'mkmf'

$CFLAGS << ' -std=c11 -Weverything -ObjC -fno-objc-arc'
$CFLAGS << ' -Wno-disabled-macro-expansion -Wno-gnu -Wno-documentation -Wno-used-but-marked-unused'
# @todo REALLY NEED TO CLEAR ALL THESE WARNINGS
$CFLAGS << ' -Wno-conversion'

$LIBS  << ' -framework Foundation'
$LIBS  << ' -framework CoreGraphics'

unless RbConfig::CONFIG['CC'].match(/clang/)
  clang = `which clang`.chomp
  fail 'Clang not installed. Cannot build C extension' if clang.empty?
  RbConfig::MAKEFILE_CONFIG['CC']  = clang
  RbConfig::MAKEFILE_CONFIG['CXX'] = clang
end

create_makefile 'accessibility/screen_shooter/screen_shooter'

makefile = File.read 'Makefile'
makefile.gsub! '$(DLLIB): $(OBJS) Makefile', '$(DLLIB): $(OBJS) Makefile ../bridge/bridge.o'
makefile.gsub! '$(LDSHARED) -o $@', ' $(LDSHARED) -o $@ ../bridge/bridge.o'
File.open('Makefile', 'w') { |f| f.write makefile }
