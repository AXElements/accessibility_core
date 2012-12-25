# v0.2.0 - Add some additional useful classes

 * Add the `Battery` module for collecting information about the battery status
 * Add the `Accessibility::Highilghter` to highlight rectangles no the screen
 * Add `NSScreen.wakeup` freedom patch to wake sleeping screens

 * Add a 99% drop-in replacement for `NSRunningApplication`
 * Add a 20% drop-in replacement for `NSWorkspace` to complement `NSRunningApplication`
 * Add a 66% drop-in replacement for `NSProcessInfo` for collecting  process info
 * Add a 66% drop-in-replacement for `NSHost` for collecting host info
 * Add a 20% drop-in replacement for `NSScreen` to help the highlighter
 * Add a 33% drop-in replacement for `NSColor` to help the highlighter

 * Various bugs fixed (wrapping strings, URLs, rectangles...)
 * Various memory leaks plugged

__NOTE__: Percentages are somewhat arbitrary based on my personal usage.

# v0.1.1 - Detect structs

 * Fix detection of struct type objects in `to_ax()`

# v0.1.0 - Initial release

 * MRI and MacRuby compatible
 * Bare minimum bridging so that accessibility-core works
