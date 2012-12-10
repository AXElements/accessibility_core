#include "ruby.h"
#import <Cocoa/Cocoa.h>

static VALUE rb_mAccessibility;
static VALUE rb_cElement;

static ID sel_new;
static ID ivar_ref;
static ID ivar_pid;

static
void
rb_acore_element_finalizer(void* obj)
{
#ifdef NOT_MACRUBY
  CFRelease((CFTypeRef)obj);
#else
  CFMakeCollectable((CFTypeRef)obj);
#endif
}

static
VALUE
rb_acore_wrap_ref(AXUIElementRef ref)
{
  return Data_Wrap_Struct(rb_cElement, NULL, rb_acore_element_finalizer, (void*)ref);
}
#define AXWRAP(x) (rb_acore_wrap_ref(x))

static
AXUIElementRef
rb_acore_unwrap_ref(VALUE obj)
{
  AXUIElementRef* ref;
  Data_Get_Struct(obj, AXUIElementRef, ref);
  // normally, we would return *ref, but that seems to fuck things up
  return (AXUIElementRef)ref;
}
#define AXUNWRAP(x) (rb_acore_unwrap_ref(x))

static
void
rb_acore_handle_error(VALUE self, OSStatus code)
{
  // TODO port the error handler from AXElements
  rb_raise(rb_eRuntimeError, "you done goofed [%d]", code);
}

/*
 * Get the application object object for an application given the
 * process identifier (PID) for that application.
 *
 * @example
 *
 *   app = Core.application_for 54743  # => #<AXUIElementRef>
 *
 * @param pid [Number]
 * @return [AXUIElementRef]
 */
static
VALUE
rb_acore_application_for(VALUE self, VALUE pid)
{
  [[NSRunLoop currentRunLoop] runUntilDate:[NSDate date]];

  pid_t the_pid = NUM2PIDT(pid);

  if ([NSRunningApplication runningApplicationWithProcessIdentifier:the_pid])
    return AXWRAP(AXUIElementCreateApplication(the_pid));
  else
    rb_raise(rb_eArgError, "pid `%d' must belong to a running application", the_pid);

  return Qnil; // unreachable
}


/*
 * Create a new reference to the system wide object. This is very useful when
 * working with the system wide object as caching the system wide reference
 * does not seem to work often.
 *
 * @example
 *
 *   system_wide  # => #<AXUIElementRefx00000000>
 *
 * @return [AXUIElementRef]
 */
static
VALUE
rb_acore_system_wide(VALUE self)
{
  return AXWRAP(AXUIElementCreateSystemWide());
}


/*
 * @todo Invalid elements do not always raise an error.
 *       This is a bug that should be logged with Apple.
 *
 * Get the list of attributes for the element
 *
 * As a convention, this method will return an empty array if the
 * backing element is no longer alive.
 *
 * @example
 *
 *   button.attributes # => ["AXRole", "AXRoleDescription", ...]
 *
 * @return [Array<String>]
 */
static
VALUE
rb_acore_attributes(VALUE self)
{
  VALUE cached_attrs = rb_ivar_get(self, ivar_ref);
  if (cached_attrs != Qnil)
    return cached_attrs;

  CFArrayRef attrs = NULL;
  VALUE        ary = 0;
  CFIndex   length = 0;
  const char* name = NULL;

  OSStatus code = AXUIElementCopyAttributeNames(AXUNWRAP(self), &attrs);
  switch (code)
    {
    case kAXErrorSuccess:
      length = CFArrayGetCount(attrs);
      ary    = rb_ary_new2(length);
      for (CFIndex idx = 0; idx < length; idx++) {
	// flying by the seat of our pants here, this hasn't failed yet
	// but probably will one day when I'm not looking
	name = CFStringGetCStringPtr(CFArrayGetValueAtIndex(attrs, idx), 0);
	if (name)
	  rb_ary_store(ary, idx, rb_str_new_cstr(name));
	else
	  // use rb_external_str_new()
	  rb_raise(rb_eRuntimeError, "NEED TO IMPLEMNET STRING COPYING");
      }
      break;
    case kAXErrorInvalidUIElement:
      ary = rb_ary_new();
      break;
    default:
      rb_acore_handle_error(self, code);
    }

  rb_ivar_set(self, ivar_ref, ary);
  return ary;
}


/*
 * Fetch the value for the given attribute
 *
 * CoreFoundation wrapped objects will be unwrapped for you, if you expect
 * to get a {CFRange} you will be given a {Range} instead.
 *
 * As a convention, if the backing element is no longer alive then
 * any attribute value will return `nil`, except for `KAXChildrenAttribute`
 * which will return an empty array. This is a debatably necessary evil,
 * inquire for details.
 *
 * If the attribute is not supported by the element then a exception
 * will be raised.
 *
 * @example
 *   window.attribute KAXTitleAttribute    # => "HotCocoa Demo"
 *   window.attribute KAXSizeAttribute     # => #<CGSize width=10.0 height=88>
 *   window.attribute KAXParentAttribute   # => #<AXUIElementRef>
 *   window.attribute KAXNoValueAttribute  # => nil
 *
 * @param name [String]
 */
static
VALUE
rb_acore_attribute(VALUE self, VALUE name)
{
  CFTypeRef        attr = NULL;
  CFStringRef attr_name = CFStringCreateWithCStringNoCopy(
							  NULL,
							  StringValueCStr(name),
							  0,
							  kCFAllocatorNull
							  );
  OSStatus code = AXUIElementCopyAttributeValue(
						AXUNWRAP(self),
						attr_name,
						&attr
						);

  switch (code)
    {
    case kAXErrorSuccess:
      CFShow(attr);
      CFRelease(attr);
      return Qtrue;
      break;
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      rb_acore_handle_error(self, code);
    }

  return Qnil; // unreachable
}


/*
 * Shortcut for getting the `kAXRoleAttribute`. Remember that
 * dead elements may return `nil` for their role.
 *
 * @example
 *
 *   window.role  # => "AXWindow"
 *
 * @return [String,nil]
 */
static
VALUE
rb_acore_role(VALUE self)
{
  CFTypeRef value = NULL;
  OSStatus   code = AXUIElementCopyAttributeValue(AXUNWRAP(self), kAXRoleAttribute, &value);

  switch (code)
    {
    case kAXErrorSuccess:
      return rb_str_new_cstr(CFStringGetCStringPtr((CFStringRef)value, 0));
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      rb_acore_handle_error(self, code);
    }

  return Qnil; // unreachable
}

/*
 * @note You might get `nil` back as the subrole as AXWebArea
 *       objects are known to do this. You need to check. :(
 *
 * Shortcut for getting the `kAXSubroleAttribute`
 *
 * @example
 *   window.subrole    # => "AXDialog"
 *   web_area.subrole  # => nil
 *
 * @return [String,nil]
 */
static
VALUE
rb_acore_subrole(VALUE self)
{
  CFTypeRef value = NULL;
  OSStatus   code = AXUIElementCopyAttributeValue(AXUNWRAP(self), kAXSubroleAttribute, &value);

  switch (code)
    {
    case kAXErrorSuccess:
      return rb_str_new_cstr(CFStringGetCStringPtr((CFStringRef)value, 0));
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      rb_acore_handle_error(self, code);
    }

  return Qnil; // unreachable
}

/*
 * Get the process identifier (PID) of the application that the element
 * belongs to.
 *
 * This method will return `0` if the element is dead or if the receiver
 * is the the system wide element.
 *
 * @example
 *
 *   window.pid               # => 12345
 *   Element.system_wide.pid  # => 0
 *
 * @return [Fixnum]
 */
static
VALUE
rb_acore_pid(VALUE self)
{
  VALUE cached_pid = rb_ivar_get(self, ivar_pid);
  if (cached_pid != Qnil)
    return cached_pid;

  pid_t     pid = 0;
  OSStatus code = AXUIElementGetPid(AXUNWRAP(self), &pid);

  switch (code)
    {
    case kAXErrorSuccess:
      break;
    case kAXErrorInvalidUIElement:
      if (CFEqual(AXUNWRAP(self), AXUIElementCreateSystemWide())) {
	pid = 0;
	break;
      }
    default:
      rb_acore_handle_error(self, code);
    }

  cached_pid = PIDT2NUM(pid);
  rb_ivar_set(self, ivar_pid, cached_pid);
  return cached_pid;
}


/*
 * Get the application object object for the receiver
 *
 * @return [AXUIElementRef]
 */
static
VALUE
rb_acore_application(VALUE self)
{
  return rb_acore_application_for(rb_cElement, rb_acore_pid(self));
}


void
Init_caccessibility()
{
  if (!AXAPIEnabled()) {
    rb_raise(rb_eRuntimeError, "\n------------------------------------------------------------------------\nUniversal Access is disabled on this machine.\n\nPlease enable it in the System Preferences.\n\nSee https://github.com/Marketcircle/AXElements#getting-setup\n------------------------------------------------------------------------\n");
  }

  sel_new  = rb_intern("new");
  ivar_ref = rb_intern("@ref");
  ivar_pid = rb_intern("@pid");

  rb_mAccessibility = rb_define_module("Accessibility");

  /*
   * Document-module: Accessibility::Element
   *
   * Core abstraction layer that that interacts with OS X Accessibility
   * APIs (AXAPI). This provides a generic object oriented mixin for
   * the low level APIs. In MacRuby, bridge support turns C structs into
   * "first class" objects. To that end, instead of adding an extra allocation
   * to wrap the object, we will simply add a mixin to add some basic
   * functionality. A more Ruby-ish wrapper is available through {AX::Element}.
   * The complication in making the mixin more fully featured is that the class
   * which we mix into is abstract and shared for a number of different C structs.
   *
   * This module is responsible for handling pointers and dealing with error
   * codes for functions that make use of them. The methods in this class
   * provide a cleaner, more Ruby-ish interface to the low level CoreFoundation
   * functions that compose AXAPI than are natively available.
   *
   * @example
   *
   *   element = AXUIElementCreateSystemWide()
   *   element.attributes                      # => ["AXRole", "AXChildren", ...]
   *   element.size_of "AXChildren"            # => 12
   */
  rb_cElement = rb_define_class_under(rb_mAccessibility, "Element", rb_cObject);
  rb_define_attr(rb_cElement, "ref", 1, 0);

  rb_define_singleton_method(rb_cElement, "application_for", rb_acore_application_for, 1);
  rb_define_singleton_method(rb_cElement, "system_wide",     rb_acore_system_wide,     0);

  rb_define_method(rb_cElement, "attributes",  rb_acore_attributes,  0);
  rb_define_method(rb_cElement, "attribute",   rb_acore_attribute,   1);
  rb_define_method(rb_cElement, "role",        rb_acore_role,        0);
  rb_define_method(rb_cElement, "subrole",     rb_acore_subrole,     0);
  rb_define_method(rb_cElement, "pid",         rb_acore_pid,         0);
  rb_define_method(rb_cElement, "application", rb_acore_application, 0);
}
