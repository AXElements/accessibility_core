# 0.0.1 - Initial Release

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
