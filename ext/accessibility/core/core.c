#include "ruby.h"
#include "../bridge/bridge.h"
#import <Cocoa/Cocoa.h>


static ID ivar_attrs;
static ID ivar_param_attrs;
static ID ivar_actions;
static ID ivar_pid;
static ID ivar_key_rate;

static ID sel_to_f;

static ID rate_very_slow;
static ID rate_slow;
static ID rate_normal;
static ID rate_default;
static ID rate_fast;
static ID rate_zomg;


static
VALUE
handle_error(VALUE self, const AXError code)
{
  @autoreleasepool {
    NSString* const description =
        (NSString* const)CFCopyDescription(unwrap_ref(self));
    [description autorelease];

    const char* const inspected_self = description.UTF8String;

    switch (code) {
    case kAXErrorSuccess:
	rb_raise(rb_eRuntimeError,
		 "internal accessibility_core error");

    case kAXErrorFailure:
	rb_raise(rb_eRuntimeError,
		 "An accessibility system failure, possibly an allocation "
		 "failure, occurred with %s; stopping to be safe",
		 inspected_self);

    case kAXErrorIllegalArgument:
	rb_raise(rb_eArgError,
		 "illegal argument was passed to the method for %s",
		 inspected_self);

    case kAXErrorInvalidUIElement:
	rb_raise(rb_eArgError,
		 "invalid element `%s' (probably dead)",
		 inspected_self);

    case kAXErrorInvalidUIElementObserver:
	rb_raise(rb_eArgError,
		 "invalid observer passed to the method for %s",
		 inspected_self);

    case kAXErrorCannotComplete:
	spin(0);

        pid_t pid = 0;
	AXUIElementGetPid(unwrap_ref(self), &pid);
	NSRunningApplication* const app =
            [NSRunningApplication runningApplicationWithProcessIdentifier:pid];

	if (app)
            rb_raise(rb_eRuntimeError,
                     "accessibility messaging failure. "
                     "Perhaps the application is busy or unresponsive?");
	else
            rb_raise(rb_eRuntimeError,
                     "application for pid=%d is no longer running. "
                     "Maybe it crashed?",
                     pid);

    case kAXErrorAttributeUnsupported:
        rb_raise(rb_eArgError, "attribute unsupported");

    case kAXErrorActionUnsupported:
        rb_raise(rb_eArgError, "action unsupported");

    case kAXErrorNotificationUnsupported:
        rb_raise(rb_eArgError, "notification unsupported");

    case kAXErrorNotImplemented:
        rb_raise(rb_eNotImpError, "method not supported by the receiver");

    case kAXErrorNotificationAlreadyRegistered:
	rb_raise(rb_eArgError, "notification has already been registered");

    case kAXErrorNotificationNotRegistered:
	rb_raise(rb_eRuntimeError, "notification is not registered yet");

    case kAXErrorAPIDisabled:
	rb_raise(rb_eRuntimeError, "AXAPI has been disabled");

    case kAXErrorNoValue:
	rb_raise(rb_eRuntimeError,
                 "accessibility_core internal error; "
		 "should be handled internally");

    case kAXErrorParameterizedAttributeUnsupported:
	rb_raise(rb_eArgError, "parameterized attribute unsupported");

    case kAXErrorNotEnoughPrecision:
	rb_raise(rb_eRuntimeError,
		 "AXAPI said there was not enough precision ¯\\(°_o)/¯");

    default:
	rb_raise(rb_eRuntimeError,
		 "accessibility_core majorly goofed [%d]",
		 (int)code);
      }
  }

  return Qnil;
}


static
VALUE
rb_acore_application_for(VALUE self, VALUE pid)
{
  NSDate* date = [NSDate date];
  [[NSRunLoop currentRunLoop] runUntilDate:date];
  [date release];

  pid_t                     the_pid = NUM2PIDT(pid);
  NSRunningApplication* running_app =
    [NSRunningApplication runningApplicationWithProcessIdentifier:the_pid];

  if (running_app) {
    VALUE app = wrap_ref(AXUIElementCreateApplication(the_pid));
    [running_app release];
      return app;
  }

  rb_raise(
	   rb_eArgError,
	   "pid `%d' must belong to a running application",
	   the_pid
	   );

  return Qnil; // unreachable
}


static
VALUE
rb_acore_system_wide(VALUE self)
{
  return wrap_ref(AXUIElementCreateSystemWide());
}


static
VALUE
rb_acore_key_rate(VALUE self)
{
  return rb_ivar_get(self, ivar_key_rate);
}


static
VALUE
rb_acore_set_key_rate(VALUE self, VALUE rate)
{
  if (TYPE(rate) == T_SYMBOL) {
    ID key_rate = SYM2ID(rate);
    if (key_rate == rate_very_slow)
      rate = DBL2NUM(0.9);
    else if (key_rate == rate_slow)
      rate = DBL2NUM(0.09);
    else if (key_rate == rate_normal || rate == rate_default)
      rate = DBL2NUM(0.009);
    else if (key_rate == rate_fast)
      rate = DBL2NUM(0.0009);
    else if (key_rate == rate_zomg)
      rate = DBL2NUM(0.00009);
    else
      rb_raise(rb_eArgError, "Unknown rate `%s'", rb_id2name(key_rate));
  }
  else {
    rate = rb_funcall(rate, sel_to_f, 0);
  }

  return rb_ivar_set(self, ivar_key_rate, rate);
}


static
int
acore_is_system_wide(VALUE other)
{
  AXUIElementRef system = AXUIElementCreateSystemWide();
  int result = CFEqual(unwrap_ref(other), system);
  CFRelease(system);
  return result;
}
#define IS_SYSTEM_WIDE(x) (acore_is_system_wide(x))


static
VALUE
rb_acore_attributes(VALUE self)
{
  VALUE cached_attrs = rb_ivar_get(self, ivar_attrs);
  if (cached_attrs != Qnil)
    return cached_attrs;

  CFArrayRef attrs = NULL;
  AXError     code = AXUIElementCopyAttributeNames(unwrap_ref(self), &attrs);
  switch (code)
    {
    case kAXErrorSuccess:
      cached_attrs = wrap_array_strings(attrs);
      rb_ivar_set(self, ivar_attrs, cached_attrs);
      CFRelease(attrs);
      return cached_attrs;
    case kAXErrorInvalidUIElement:
      return rb_ary_new();
    default:
      // TODO we should actually allow for a grace period and try again in
      //      every case where would be deferring to the default error handler,
      //      and maybe even in every case where we get a non-zero result code
      //
      //      WE SHOULD HANDLE THINGS LIKE FAILURE AND CANNOT COMPLETE LIKE THIS
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_attribute(VALUE self, VALUE name)
{
  VALUE             obj;
  CFTypeRef        attr = NULL;
  CFStringRef attr_name = unwrap_string(name);
  AXError          code = AXUIElementCopyAttributeValue(
							unwrap_ref(self),
							attr_name,
							&attr
							);
  CFRelease(attr_name);
  switch (code)
    {
    case kAXErrorSuccess:
      obj = to_ruby(attr);
      if (TYPE(obj) != T_DATA)
        CFRelease(attr);
      return obj;
    case kAXErrorFailure:
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
    case kAXErrorAttributeUnsupported:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_size_of(VALUE self, VALUE name)
{
  CFIndex          size = 0;
  CFStringRef attr_name = unwrap_string(name);
  AXError          code = AXUIElementGetAttributeValueCount(
							    unwrap_ref(self),
							    attr_name,
							    &size
							    );
  CFRelease(attr_name);
  switch (code)
    {
    case kAXErrorSuccess:
      return INT2FIX(size);
    case kAXErrorFailure:
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return INT2FIX(0);
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_is_writable(VALUE self, VALUE name)
{
  Boolean        result;
  CFStringRef attr_name = unwrap_string(name);
  AXError          code = AXUIElementIsAttributeSettable(
							unwrap_ref(self),
							attr_name,
							&result
							);
  CFRelease(attr_name);
  switch (code)
    {
    case kAXErrorSuccess:
      return (result ? Qtrue : Qfalse);
    case kAXErrorFailure:
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qfalse;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_set(VALUE self, VALUE name, VALUE value)
{
  CFTypeRef    ax_value = to_ax(value);
  CFStringRef attr_name = unwrap_string(name);
  AXError          code = AXUIElementSetAttributeValue(
						       unwrap_ref(self),
						       attr_name,
						       ax_value
						       );
  CFRelease(ax_value);
  switch (code)
    {
    case kAXErrorSuccess:
      return value;
    default:
      return handle_error(self, code); // name, value
    }
}


static
VALUE
rb_acore_role(VALUE self)
{
  VALUE       obj;
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXRoleAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      obj = wrap_string(value);
      CFRelease(value);
      return obj;
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_subrole(VALUE self)
{
  VALUE       obj;
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXSubroleAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      if (value) {
	obj = wrap_string(value);
	CFRelease(value);
	return obj;
      }
      return Qnil;
    case kAXErrorFailure:
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
    case kAXErrorAttributeUnsupported:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_parent(VALUE self)
{
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXParentAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      return wrap_ref(value);
    case kAXErrorFailure:
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
    case kAXErrorAttributeUnsupported:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_children(VALUE self)
{
  VALUE       obj;
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXChildrenAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      obj = wrap_array_refs(value);
      CFRelease(value);
      return obj;
    case kAXErrorFailure:
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return rb_ary_new();
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_value(VALUE self)
{
  VALUE       obj;
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXValueAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      obj = to_ruby(value);
      if (TYPE(obj) != T_DATA)
        CFRelease(value);
      return obj;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_pid(VALUE self)
{
  VALUE cached_pid = rb_ivar_get(self, ivar_pid);
  if (cached_pid != Qnil)
    return cached_pid;

  pid_t    pid = 0;
  AXError code = AXUIElementGetPid(unwrap_ref(self), &pid);

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
      handle_error(self, code);
    }

  cached_pid = PIDT2NUM(pid);
  rb_ivar_set(self, ivar_pid, cached_pid);
  return cached_pid;
}


static
VALUE
rb_acore_parameterized_attributes(VALUE self)
{
  VALUE cached_attrs = rb_ivar_get(self, ivar_param_attrs);
  if (cached_attrs != Qnil)
    return cached_attrs;

  CFArrayRef attrs = NULL;
  AXError     code = AXUIElementCopyParameterizedAttributeNames(
                                                                unwrap_ref(self),
								&attrs
								);
  switch (code)
    {
    case kAXErrorSuccess:
      cached_attrs = wrap_array_strings(attrs);
      rb_ivar_set(self, ivar_param_attrs, cached_attrs);
      CFRelease(attrs);
      return cached_attrs;
    case kAXErrorInvalidUIElement:
      return rb_ary_new();
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_parameterized_attribute(VALUE self, VALUE name, VALUE parameter)
{
  VALUE             obj;
  CFTypeRef       param = to_ax(parameter);
  CFTypeRef        attr = NULL;
  CFStringRef attr_name = unwrap_string(name);
  AXError          code = AXUIElementCopyParameterizedAttributeValue(
								     unwrap_ref(self),
								     attr_name,
								     param,
								     &attr
								     );
  CFRelease(param);
  CFRelease(attr_name);
  switch (code)
    {
    case kAXErrorSuccess:
      obj = to_ruby(attr);
      if (TYPE(obj) != T_DATA)
        CFRelease(attr);
      return obj;
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_actions(VALUE self)
{
  VALUE cached_actions = rb_ivar_get(self, ivar_actions);
  if (cached_actions != Qnil)
    return cached_actions;

  CFArrayRef actions = NULL;
  AXError       code = AXUIElementCopyActionNames(unwrap_ref(self), &actions);
  switch (code)
    {
    case kAXErrorSuccess:
      cached_actions = wrap_array_strings(actions);
      rb_ivar_set(self, ivar_actions, cached_actions);
      CFRelease(actions);
      return cached_actions;
    case kAXErrorInvalidUIElement:
      return rb_ary_new();
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_perform(VALUE self, VALUE name)
{
  CFStringRef action = unwrap_string(name);
  AXError       code = AXUIElementPerformAction(unwrap_ref(self), action);

  CFRelease(action);
  switch (code)
    {
    case kAXErrorSuccess:
      return Qtrue;
    case kAXErrorInvalidUIElement:
      return Qfalse;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_post(VALUE self, VALUE events)
{
#if MAC_OS_X_VERSION_MIN_ALLOWED <= MAC_OS_X_VERSION_10_9
  rb_raise(rb_eRuntimeError, "Posting keyboard events is deprecated in 10.9 and later");
  return Qundef;
#else
  events = rb_ary_to_ary(events);
  long length = RARRAY_LEN(events);
  useconds_t sleep_time = NUM2DBL(rb_ivar_get(rb_cElement, ivar_key_rate)) * 100000;

  // CGCharCode key_char = 0; // TODO this value seems to not matter?
  VALUE            pair;
  CGKeyCode virtual_key;
  int         key_state;
  AXError          code;


  for (long i = 0; i < length; i++) {
    pair        = rb_ary_entry(events, i);
    virtual_key = NUM2INT(rb_ary_entry(pair, 0));
    key_state   = rb_ary_entry(pair, 1) == Qtrue ? true : false;
    code        = AXUIElementPostKeyboardEvent(
					       unwrap_ref(self),
					       0,
					       virtual_key,
					       key_state
					       );
    switch (code)
      {
      case kAXErrorSuccess:
	break;
      default:
	handle_error(self, code);
      }

    usleep(sleep_time);
  }

  return self;
#endif
}


static
VALUE
rb_acore_is_invalid(VALUE self)
{
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXRoleAttribute,
						  &value
						  );
  if (value)
    CFRelease(value);
  return (code == kAXErrorInvalidUIElement ? Qtrue : Qfalse);
}


static
VALUE
rb_acore_application(VALUE self)
{
  return rb_acore_application_for(rb_cElement, rb_acore_pid(self));
}


static
VALUE
rb_acore_set_timeout_to(VALUE self, VALUE seconds)
{
  float timeout = NUM2DBL(seconds);
  AXError  code = AXUIElementSetMessagingTimeout(unwrap_ref(self), timeout);

  switch (code)
    {
    case kAXErrorSuccess:
      return seconds;
    default:
      return handle_error(self, code); // seconds
    }
}


static
VALUE
rb_acore_element_at(VALUE self, VALUE point)
{
  if (self == rb_cElement)
    self = rb_acore_system_wide(self);

  AXUIElementRef ref = NULL;
  CGPoint          p = unwrap_point(point);
  AXError       code = AXUIElementCopyElementAtPosition(
							unwrap_ref(self),
							p.x,
							p.y,
							&ref
							);
  switch (code)
    {
    case kAXErrorSuccess:
      return wrap_ref(ref);
    case kAXErrorNoValue:
      return Qnil;
    case kAXErrorInvalidUIElement:
      if (!IS_SYSTEM_WIDE(self))
	return rb_acore_element_at(rb_acore_system_wide(rb_cElement), point);
      else
	return Qnil;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_equality(VALUE self, VALUE other)
{
  if (CLASS_OF(other) == rb_cElement)
    if (CFEqual(unwrap_ref(self), unwrap_ref(other)))
      return Qtrue;
  return Qfalse;
}


void
Init_core()
{
  Init_bridge();

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_9
  if (!AXAPIEnabled())
    rb_raise(
	     rb_eRuntimeError,
	     "\n"                                                                         \
	     "------------------------------------------------------------------------\n" \
	     "Universal Access is disabled on this machine.\n"                            \
	     "Please enable it in the System Preferences.\n"                              \
	     "See https://github.com/Marketcircle/AXElements#getting-setup\n"             \
	     "------------------------------------------------------------------------\n"
	     );
#else
    CFMutableDictionaryRef options = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
     CFDictionarySetValue(options, kAXTrustedCheckOptionPrompt, kCFBooleanTrue);

    if (!AXIsProcessTrustedWithOptions(options)) {
      rb_raise(rb_eRuntimeError,
               "\n"
               "-------------------------------------------------------------------\n" \
               "The Application that is running AXElements is not trused to control\n" \
               "your computer. A window prompting you to grant permission to the\n"     \
               "application should appear right now. Please grant the application\n"   \
               "permission to control your computer and try again.\n"                   \
               "-------------------------------------------------------------------");
    }
     CFRelease(options);


#endif
  // bs that needs to be initialized from the bridge.c import
  sel_x        = rb_intern("x");
  sel_y        = rb_intern("y");
  sel_width    = rb_intern("width");
  sel_height   = rb_intern("height");
  sel_origin   = rb_intern("origin");
  sel_size     = rb_intern("size");
  sel_to_point = rb_intern("to_point");
  sel_to_size  = rb_intern("to_size");
  sel_to_s     = rb_intern("to_s");

  rb_mAccessibility = rb_const_get(rb_cObject, rb_intern("Accessibility"));
  rb_cElement       = rb_define_class_under(rb_mAccessibility, "Element", rb_cObject);
  rb_cCGPoint       = rb_const_get(rb_cObject, rb_intern("CGPoint"));
  rb_cCGSize        = rb_const_get(rb_cObject, rb_intern("CGSize"));
  rb_cCGRect        = rb_const_get(rb_cObject, rb_intern("CGRect"));


  // these should be defined by now
  rb_mAccessibility = rb_const_get(rb_cObject, rb_intern("Accessibility"));
  rb_cElement = rb_define_class_under(rb_mAccessibility, "Element", rb_cObject);

  ivar_attrs       = rb_intern("@attrs");
  ivar_param_attrs = rb_intern("@param_attrs");
  ivar_actions     = rb_intern("@actions");
  ivar_pid         = rb_intern("@pid");
  ivar_key_rate    = rb_intern("@key_rate");

  rb_define_singleton_method(rb_cElement, "application_for", rb_acore_application_for,          1);
  rb_define_singleton_method(rb_cElement, "system_wide",     rb_acore_system_wide,              0);
  rb_define_singleton_method(rb_cElement, "element_at",      rb_acore_element_at,               1);
  rb_define_singleton_method(rb_cElement, "key_rate",        rb_acore_key_rate,                 0);
  rb_define_singleton_method(rb_cElement, "key_rate=",       rb_acore_set_key_rate,             1);

  sel_to_f       = rb_intern("to_f");
  rate_very_slow = rb_intern("very_slow");
  rate_slow      = rb_intern("slow");
  rate_normal    = rb_intern("normal");
  rate_default   = rb_intern("default");
  rate_fast      = rb_intern("fast");
  rate_zomg      = rb_intern("zomg");
  rb_acore_set_key_rate(rb_cElement, DBL2NUM(0.009)); // initialize the value right now

  rb_define_method(rb_cElement, "attributes",                rb_acore_attributes,               0);
  rb_define_method(rb_cElement, "attribute",                 rb_acore_attribute,                1);
  rb_define_method(rb_cElement, "size_of",                   rb_acore_size_of,                  1);
  rb_define_method(rb_cElement, "writable?",                 rb_acore_is_writable,              1);
  rb_define_method(rb_cElement, "set",                       rb_acore_set,                      2);

  rb_define_method(rb_cElement, "role",                      rb_acore_role,                     0);
  rb_define_method(rb_cElement, "subrole",                   rb_acore_subrole,                  0);
  rb_define_method(rb_cElement, "parent",                    rb_acore_parent,                   0);
  rb_define_method(rb_cElement, "children",                  rb_acore_children,                 0);
  rb_define_method(rb_cElement, "value",                     rb_acore_value,                    0);
  rb_define_method(rb_cElement, "pid",                       rb_acore_pid,                      0);

  rb_define_method(rb_cElement, "parameterized_attributes",  rb_acore_parameterized_attributes, 0);
  rb_define_method(rb_cElement, "parameterized_attribute",   rb_acore_parameterized_attribute,  2);

  rb_define_method(rb_cElement, "actions",                   rb_acore_actions,                  0);
  rb_define_method(rb_cElement, "perform",                   rb_acore_perform,                  1);
  rb_define_method(rb_cElement, "post",                      rb_acore_post,                     1);

  rb_define_method(rb_cElement, "invalid?",                  rb_acore_is_invalid,               0);
  rb_define_method(rb_cElement, "set_timeout_to",            rb_acore_set_timeout_to,           1);
  // TODO make this meaningful, currently has no effect on calling rb_acore_post
  rb_define_method(rb_cElement, "key_rate",                  rb_acore_key_rate,                 0);
  rb_define_method(rb_cElement, "key_rate=",                 rb_acore_set_key_rate,             1);
  rb_define_method(rb_cElement, "application",               rb_acore_application,              0);
  rb_define_method(rb_cElement, "element_at",                rb_acore_element_at,               1);
  rb_define_method(rb_cElement, "==",                        rb_acore_equality,                 1);

}
