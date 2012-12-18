# accessibility\_core

A port of accessibility/core.rb from [AXElements](http://github.com/Marketcircle/AXElements),
but cleaned up and rewritten in C to be more portable across languages and
runtimes.

[Documentation](http://rdoc.info/gems/accessibility_core/frames)


## Examples

    require 'accessibility/core'

    app = Accessibility::Element.application_for 276  # PID for some app
    app.attributes

    window = app.main_window
    window.attributes
    window.attribute 'AXPosition'
    window.set 'AXPosition', CGPoint.new(100, 100)


## Note

At this point, `rake-compiler` does not run on MacRuby
'out-of-the-box'. Actually, it does, but it declares a dependency on
rake which causes rake to be installed from rubygems. Rake from
rubygems does not work with MacRuby. So you need to either fix the
probem with MacRuby or just hack the `rake-compiler.gemspec` file
after you install `rake-compiler` so it does not think it depends on
rake anymore.


## TODO

  * bridging for `NSAttributedString`
  * more descriptive error handling for the C extension

## Copyright

Copyright (c) 2012, Mark Rada
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Mark Rada nor the names of its
  contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Mark Rada BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
