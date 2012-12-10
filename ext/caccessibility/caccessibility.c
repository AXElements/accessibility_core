#include "ruby.h"
#import <Cocoa/Cocoa.h>

static VALUE rb_mAccessibility;
static VALUE rb_cElement;
static VALUE rb_cCGPoint;

static ID sel_new;
static ID sel_x;
static ID sel_y;
static ID sel_to_point;
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
#define UNAX(x) (rb_acore_wrap_ref(x))

static
AXUIElementRef
rb_acore_unwrap_ref(VALUE obj)
{
  AXUIElementRef* ref;
  Data_Get_Struct(obj, AXUIElementRef, ref);
  // normally, we would return *ref, but that seems to fuck things up
  return (AXUIElementRef)ref;
}
#define AX(x) (rb_acore_unwrap_ref(x))

static
VALUE
rb_acore_wrap_point(CGPoint point)
{
  // TODO: Data_Wrap_Struct instead
#if NOT_MACRUBY
  return rb_struct_new(rb_cCGPoint, DBL2NUM(point.x), DBL2NUM(point.y));
#else
  return rb_funcall(rb_cCGPoint, sel_new, 2, DBL2NUM(point.x), DBL2NUM(point.y));
#endif
}
#define UNPOINT(x) (rb_acore_wrap_point(x))

static
CGPoint
rb_acore_unwrap_point(VALUE point)
{
  point = rb_funcall(point, sel_to_point, 0);

#if NOT_MACRUBY
  double x = NUM2DBL(rb_struct_getmember(point, sel_x));
  double y = NUM2DBL(rb_struct_getmember(point, sel_y));
  return CGPointMake(x, y);

#else
  CGPoint* ptr;
  Data_Get_Struct(point, CGPoint, ptr);
  return *ptr;

#endif
}
#define POINT(x) (rb_acore_unwrap_point(x))

static inline
VALUE
rb_acore_wrap_value_point(AXValueRef value)
{
  CGPoint point;
  AXValueGetValue(value, kAXValueCGPointType, &point);
  return UNPOINT(point);
}

static
VALUE
rb_acore_wrap_value(AXValueRef value)
{
  switch (AXValueGetType(value))
    {
    case kAXValueIllegalType:
      // TODO better error message
      rb_raise(rb_eArgError, "herped when you should have derped");
    case kAXValueCGPointType:
      return rb_acore_wrap_value_point(value);
    case kAXValueCGSizeType:
    case kAXValueCGRectType:
    case kAXValueCFRangeType:
    case kAXValueAXErrorType:
      break;
    default:
      rb_bug("You've found a bug in something...not sure who to blame");
    }

  return Qnil; // unreachable
}
#define UNVALUE(x) (rb_acore_wrap_value(x))

#define IS_SYSTEM_WIDE(x) (CFEqual(AX(x), AXUIElementCreateSystemWide()))

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
  // TODO release memory instead of leaking it (whole function is a leak)
  [[NSRunLoop currentRunLoop] runUntilDate:[NSDate date]];

  pid_t the_pid = NUM2PIDT(pid);

  if ([NSRunningApplication runningApplicationWithProcessIdentifier:the_pid])
    return UNAX(AXUIElementCreateApplication(the_pid));
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
  return UNAX(AXUIElementCreateSystemWide());
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

  OSStatus code = AXUIElementCopyAttributeNames(AX(self), &attrs);
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
						AX(self),
						attr_name,
						&attr
						);

  switch (code)
    {
    case kAXErrorSuccess:
      CFShow(attr);
      // CFRelease(attr); // LEAK!
      if (CFGetTypeID(attr) == AXValueGetTypeID())
	return UNVALUE(attr);
      return Qtrue;
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
  OSStatus   code = AXUIElementCopyAttributeValue(AX(self), kAXRoleAttribute, &value);

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
  OSStatus   code = AXUIElementCopyAttributeValue(AX(self), kAXSubroleAttribute, &value);

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
  OSStatus code = AXUIElementGetPid(AX(self), &pid);

  switch (code)
    {
    case kAXErrorSuccess:
      break;
    case kAXErrorInvalidUIElement:
      if (IS_SYSTEM_WIDE(self)) {
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


/*
 * Find the top most element at the given point on the screen
 *
 * If the receiver is a regular application or element then the return
 * will be specific to the application. If the receiver is the system
 * wide object then the return is the top most element regardless of
 * application.
 *
 * The coordinates should be specified using the flipped coordinate
 * system (origin is in the top-left, increasing downward and to the right
 * as if reading a book in English).
 *
 * If more than one element is at the position then the z-order of the
 * elements will be used to determine which is "on top".
 *
 * This method will safely return `nil` if there is no UI element at the
 * give point.
 *
 * @example
 *
 *   Element.system_wide.element_at [453, 200]  # table
 *   app.element_at CGPoint.new(453, 200)       # table
 *
 * @param point [CGPoint,#to_point]
 * @return [AXUIElementRef,nil]
 */
static
VALUE
rb_acore_element_at(VALUE self, VALUE point)
{
  AXUIElementRef ref = NULL;
  CGPoint          p = POINT(point);
  OSStatus      code = AXUIElementCopyElementAtPosition(AX(self), p.x, p.y, &ref);

  switch (code)
    {
    case kAXErrorSuccess:
      return UNAX(ref);
    case kAXErrorNoValue:
      return Qnil;
    case kAXErrorInvalidUIElement:
      if (!IS_SYSTEM_WIDE(self))
	return rb_acore_element_at(rb_acore_system_wide(rb_cElement), point);
      else
	return Qnil;
    default:
      rb_acore_handle_error(self, code); // point, nil, nil
    }

  return Qnil; // unreachable
}


/*
 * Change the timeout value for the given element
 *
 * If you change the timeout on the system wide object, it affets all timeouts.
 *
 * Setting the global timeout to `0` seconds will reset the timeout value
 * to the system default. The system default timeout value is `6 seconds`
 * as of the writing of this documentation, but Apple has not publicly
 * documented this (we had to ask in person at WWDC).
 *
 * @param seconds [Number]
 * @return [Number]
 */
static
VALUE
rb_acore_set_timeout_to(VALUE self, VALUE seconds)
{
  float timeout = NUM2DBL(seconds);
  OSStatus code = AXUIElementSetMessagingTimeout(AX(self), timeout);

  switch (code)
    {
    case kAXErrorSuccess:
      return seconds;
    default:
      rb_acore_handle_error(self, code); // seconds
    }

  return Qnil; // unreachable
}


void
Init_caccessibility()
{
  if (!AXAPIEnabled()) {
    rb_raise(rb_eRuntimeError, "\n------------------------------------------------------------------------\nUniversal Access is disabled on this machine.\n\nPlease enable it in the System Preferences.\n\nSee https://github.com/Marketcircle/AXElements#getting-setup\n------------------------------------------------------------------------\n");
  }

  sel_new      = rb_intern("new");
  sel_x        = rb_intern("x");
  sel_y        = rb_intern("y");
  sel_to_point = rb_intern("to_point");

  ivar_ref = rb_intern("@ref");
  ivar_pid = rb_intern("@pid");

  // on either supported Ruby, these should be defined by now
  rb_cCGPoint       = rb_const_get(rb_cObject, rb_intern("CGPoint"));
  rb_mAccessibility = rb_const_get(rb_cObject, rb_intern("Accessibility"));


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

  rb_define_method(rb_cElement, "attributes",     rb_acore_attributes,     0);
  rb_define_method(rb_cElement, "attribute",      rb_acore_attribute,      1);
  rb_define_method(rb_cElement, "role",           rb_acore_role,           0);
  rb_define_method(rb_cElement, "subrole",        rb_acore_subrole,        0);
  rb_define_method(rb_cElement, "pid",            rb_acore_pid,            0);
  rb_define_method(rb_cElement, "application",    rb_acore_application,    0);
  rb_define_method(rb_cElement, "element_at",     rb_acore_element_at,     1);
  rb_define_method(rb_cElement, "set_timeout_to", rb_acore_set_timeout_to, 1);
}
