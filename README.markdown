# accessibility\_core

A port of accessibility/core.rb from [AXElements](http://github.com/Marketcircle/AXElements),
but cleaned up and rewritten in C to be more portable across languages and
runtimes.

[Documentation](http://rdoc.info/gems/accessibility_core/frames)

[![Dependency Status](https://gemnasium.com/AXElements/accessibility_core.png)](https://gemnasium.com/AXElements/accessibility_core)
[![Code Climate](https://codeclimate.com/github/AXElements/accessibility_core.png)](https://codeclimate.com/github/AXElements/accessibility_core)


## Examples

    require 'accessibility/core'

    app = Accessibility::Element.application_for 276  # PID for some app
    app.attributes

    window = app.attribute 'AXMainWindow'
    window.attributes
    window.attribute 'AXPosition'
    window.set 'AXPosition', CGPoint.new(100, 100)


## TODO

  * more descriptive error handling for the C extension
  * handle string encodings that are not UTF8 (or a subset of UTF8)


## Tests

Currently, the tests are all run as one task. You can run individual
test files if needed. This is not ideal, but not a high priority for
me right now.

To properly run tests, you must run them under CRuby and also under
MacRuby and get the same results.


## Copyright

Copyright (c) 2012-2013, Mark Rada
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


[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/AXElements/accessibility_core/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

