# 0.3.2 - Adjustments for AXElements

  * Fix `Element#children` raising when AXAPI returns failure error code
  * Conditionally defined `CGPoint`/`CGSize`/`CGRect` on MRI
  * Tweak various bits of documentation related to Cocoa mappings
  * Calculate `NSString` length properly when wrapping in the C extension
  * Add a 10% drop-in replacement for `NSBundle` for AXElements
  * Add `wrap_dictionary` to wrap `NSDictionary` objects
  * Force `CGPoint`/`CGSize` attrs to be `Float` type at initialize time
  * Add `Object#to_ruby` when running on MRI


# 0.3.1 - Fix it up

  * Fix C extensions being installed to the wrong location
  * Fix Accessibility module not always being defined at the appropriate time


# 0.3.0 - Merge accessibility\_bridge

  * Merge in `accessibility_bridge` v0.2.0

    - Add the `Battery` module for collecting information about the battery status
    - Add the `Accessibility::Highilghter` module to highlight rectangles on the screen
    - Add `NSScreen.wakeup` method as a freedom patch to wake sleeping screens

    - Add a 99% drop-in replacement for `NSRunningApplication`
    - Add a 20% drop-in replacement for `NSWorkspace` to complement `NSRunningApplication`
    - Add a 66% drop-in replacement for `NSProcessInfo` for collecting  process info
    - Add a 66% drop-in-replacement for `NSHost` for collecting host info
    - Add a 20% drop-in replacement for `NSScreen` to help the highlighter
    - Add a 33% drop-in replacement for `NSColor` to help the highlighter

    - Various bugs fixed (wrapping strings, URLs, rectangles...)
    - Various memory leaks plugged

    __NOTE__: Percentages are somewhat arbitrary based on my personal usage.


# 0.2.0 - Update accessibility\_bridge

  * Update bridge dependency to v0.2.0


# 0.1.2 - Fix some bridging stuff

  * Update bridge dependency to get a fix


# 0.1.1 - Fix the silly things release

  * Fix documentation not being generated
  * Fix C extension not being installed to the proper location
  * Remove redundant core extensions provided by `accessibility_bridge`


# 0.1.0 - Initial Release

  * CRuby and MacRuby compatible

  * Added `Accessibility` namespace
  * Added `Accessibility::Element` class/module as wrapper for `AXUIElementRef` structs

  * Added `Accessibility::Element.application_for` that takes a pid and returns an app reference
  * Added `Accessibility::Element.system_wide` which returns the reference for the system wide reference
  * Added `Accessibility::Element.element_at` which returns the reference for the element at the given co-ordinates
  * Added `Accessibility::Element.key_rate` for querying the typing speed
  * Added `Accessibility::Element.key_rate=` for setting the typing speed

  * Added `Accessibility::Element#attributes` for getting a list of the elements's plain attributes
  * Added `Accessibility::Element#attribute` for getting the value of an attribute
  * Added `Accessibility::Element#size_of` for getting the size of an array attribute
  * Added `Accessibility::Element#writable?` for checking writability of an attribute
  * Added `Accessibility::Element#set` for setting the value of an attribute
  * Added `Accessibility::Element#role` for getting the value of the role attribute
  * Added `Accessibility::Element#subrole` for getting the value of the subrole attribute
  * Added `Accessibility::Element#parent` for getting the value of the parent attribute
  * Added `Accessibility::Element#children` for getting the value of the children attribute
  * Added `Accessibility::Element#value` for getting the value of the value attribute
  * Added `Accessibility::Element#pid` for getting the process identifier for the app for an element
  * Added `Accessibility::Element#parameterized_attributes` for getting the list of the element's parameterized attributes
  * Added `Accessibility::Element#parameterized_attribute` for getting the value of a parameterized attribute
  * Added `Accessibility::Element#actions` for getting the list of actions an element can perform
  * Added `Accessibility::Element#perform` for telling an element to perform an action
  * Added `Accessibility::Element#post` for posting key events to an application
  * Added `Accessibility::Element#invalid?` for checking if an element is alive or dead
  * Added `Accessibility::Element#set_timeout_to` for overriding the default messaging timeout
  * Added `Accessibility::Element#application` for getting the toplevel element for an arbitrary element
  * Added `Accessibility::Element#element_at` for getting app specific element at arbitrary co-ordinates
  * Added `Accessibility::Element#==` for testing equality of elements

  * Depend on `accessibility_bridge` for bridging needs and core extensions
