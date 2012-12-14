#include "ruby.h"
#import <Cocoa/Cocoa.h>

static VALUE rb_mAccessibility;
static VALUE rb_cElement;

static VALUE rb_cCGPoint;
static VALUE rb_cCGSize;
static VALUE rb_cCGRect;
static VALUE rb_mURI; // URI module
static VALUE rb_cURI; // URI::Generic class

static ID sel_new;
static ID sel_parse;
static ID sel_x;
static ID sel_y;
static ID sel_width;
static ID sel_height;
static ID sel_origin;
static ID sel_size;
static ID sel_to_point;
static ID sel_to_size;
static ID sel_to_rect;
static ID sel_to_range;

static ID ivar_attrs;
static ID ivar_param_attrs;
static ID ivar_actions;
static ID ivar_pid;

static CFTypeID array_type;
static CFTypeID ref_type;
static CFTypeID value_type;
static CFTypeID string_type;
static CFTypeID number_type;
static CFTypeID boolean_type;
static CFTypeID url_type;
static CFTypeID date_type;

#ifdef NOT_MACRUBY
#define RELEASE(x) CFRelease(x)
#else
#define RELEASE(x) CFMakeCollectable(x)
#endif

static
void
ref_finalizer(void* obj)
{
  RELEASE((CFTypeRef)obj);
}

#define WRAP_ARRAY(wrapper) do {				\
  CFIndex length = CFArrayGetCount(array);			\
  VALUE      ary = rb_ary_new2(length);				\
								\
  for (CFIndex idx = 0; idx < length; idx++)			\
    rb_ary_store(						\
		 ary,						\
		 idx,						\
		 wrapper(CFArrayGetValueAtIndex(array, idx))	\
		);	                                        \
  return ary;							\
} while (false);


static inline
VALUE
wrap_point(CGPoint point)
{
#if NOT_MACRUBY
  return rb_struct_new(rb_cCGPoint, DBL2NUM(point.x), DBL2NUM(point.y));
#else
  return rb_funcall(rb_cCGPoint, sel_new, 2, DBL2NUM(point.x), DBL2NUM(point.y));
#endif
}

static
CGPoint
unwrap_point(VALUE point)
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

static inline
VALUE
wrap_size(CGSize size)
{
#if NOT_MACRUBY
  return rb_struct_new(rb_cCGSize, DBL2NUM(size.width), DBL2NUM(size.height));
#else
  return rb_funcall(rb_cCGSize, sel_new, 2, DBL2NUM(size.width), DBL2NUM(size.height));
#endif
}

static
CGSize
unwrap_size(VALUE size)
{
  size = rb_funcall(size, sel_to_size, 0);

#if NOT_MACRUBY
  double width  = NUM2DBL(rb_struct_getmember(size, sel_width));
  double height = NUM2DBL(rb_struct_getmember(size, sel_height));
  return CGSizeMake(width, height);

#else
  CGSize* ptr;
  Data_Get_Struct(point, CGSize, ptr);
  return *ptr;

#endif
}


static inline
VALUE
wrap_rect(CGRect rect)
{
  VALUE point = wrap_point(rect.origin);
  VALUE  size = wrap_size(rect.size);

#if NOT_MACRUBY
  return rb_struct_new(rb_cCGRect, point, size);
#else
  return rb_funcall(rb_cCGRect, sel_new, 2, point, size);
#endif
}

static
CGRect
unwrap_rect(VALUE rect)
{
  rect = rb_funcall(rect, sel_to_rect, 0);

#if NOT_MACRUBY
  CGPoint origin = unwrap_point(rb_struct_getmember(rect, sel_origin));
  CGSize    size = unwrap_size(rb_struct_getmember(rect, sel_size));
  return CGRectMake(origin.x, origin.y, size.width, size.height);

#else
  CGRect* ptr;
  Data_Get_Struct(point, CGRect, ptr);
  return *ptr;

#endif
}


static inline
VALUE
convert_cf_range(CFRange range)
{
  return rb_range_new(range.location, range.length, 0);
}

static
CFRange
convert_rb_range(VALUE range)
{
  VALUE b, e;
  int exclusive;

  range = rb_funcall(range, sel_to_range, 0);
  rb_range_values(range, &b, &e, &exclusive);

  int begin = NUM2INT(b);
  int   end = NUM2INT(e);

  if (begin < 0 || end < 0)
    // We don't know what the max length of the range will be, so we
    // can't count backwards.
    rb_raise(
	     rb_eArgError,
	     "negative values are not allowed in ranges " \
	     "that are converted to CFRange structures."
	     );

  int length = exclusive ? end-begin : end-begin + 1;
  return CFRangeMake(begin, length);
}


static
VALUE
wrap_value_point(AXValueRef value)
{
  CGPoint point;
  AXValueGetValue(value, kAXValueCGPointType, &point);
  return wrap_point(point);
}

static
VALUE
wrap_value_size(AXValueRef value)
{
  CGSize size;
  AXValueGetValue(value, kAXValueCGSizeType, &size);
  return wrap_size(size);
}

static
VALUE
wrap_value_rect(AXValueRef value)
{
  CGRect rect;
  AXValueGetValue(value, kAXValueCGRectType, &rect);
  return wrap_rect(rect);
}

static
VALUE
wrap_value_range(AXValueRef value)
{
  CFRange range;
  AXValueGetValue(value, kAXValueCFRangeType, &range);
  return convert_cf_range(range);
}

static
VALUE
wrap_value_error(AXValueRef value)
{
  OSStatus code;
  AXValueGetValue(value, kAXValueAXErrorType, &code);
  return INT2NUM(code);
}

static
VALUE
wrap_value(AXValueRef value)
{
  switch (AXValueGetType(value))
    {
    case kAXValueIllegalType:
      // TODO better error message
      rb_raise(rb_eArgError, "herped when you should have derped");
    case kAXValueCGPointType:
      return wrap_value_point(value);
    case kAXValueCGSizeType:
      return wrap_value_size(value);
    case kAXValueCGRectType:
      return wrap_value_rect(value);
    case kAXValueCFRangeType:
      return wrap_value_range(value);
    case kAXValueAXErrorType:
      return wrap_value_error(value);
    default:
      rb_bug("You've found a bug in something...not sure who to blame");
    }

  return Qnil; // unreachable
}

static inline
AXValueRef
unwrap_value_point(VALUE value)
{
  CGPoint point = unwrap_point(value);
  return AXValueCreate(kAXValueCGPointType, &point);
}

static inline
AXValueRef
unwrap_value_size(VALUE value)
{
  CGSize size = unwrap_size(value);
  return AXValueCreate(kAXValueCGSizeType, &size);
}

static inline
AXValueRef
unwrap_value_rect(VALUE value)
{
  CGRect rect = unwrap_rect(value);
  return AXValueCreate(kAXValueCGRectType, &rect);
}

static inline
AXValueRef
unwrap_value_range(VALUE value)
{
  CFRange range = convert_rb_range(value);
  return AXValueCreate(kAXValueCFRangeType, &range);
}

static
AXValueRef
unwrap_value(VALUE value)
{
  VALUE type = CLASS_OF(value);
  if (type == rb_cCGPoint)
    return unwrap_value_point(value);
  else if (type == rb_cCGSize)
    return unwrap_value_size(value);
  else if (type == rb_cCGRect)
    return unwrap_value_rect(value);
  else if (type == rb_cRange)
    return unwrap_value_range(value);

  rb_raise(rb_eArgError, "could not wrap %s", rb_class2name(type));
  return NULL; // unreachable
}

static VALUE wrap_array_values(CFArrayRef array) { WRAP_ARRAY(wrap_value) }


static inline
VALUE
wrap_ref(AXUIElementRef ref)
{
  return Data_Wrap_Struct(rb_cElement, NULL, ref_finalizer, (void*)ref);
}

static inline
AXUIElementRef
unwrap_ref(VALUE obj)
{
  AXUIElementRef* ref;
  Data_Get_Struct(obj, AXUIElementRef, ref);
  // normally, we would return *ref, but that seems to fuck things up
  return (AXUIElementRef)ref;
}

static VALUE wrap_array_refs(CFArrayRef array) { WRAP_ARRAY(wrap_ref) }


static inline
VALUE
wrap_string(CFStringRef string)
{
  // flying by the seat of our pants here, this hasn't failed yet
  // but probably will one day when I'm not looking
  const char* name = CFStringGetCStringPtr(string, kCFStringEncodingMacRoman);
  if (name)
    return rb_str_new(name, CFStringGetLength(string));
  else
    // use rb_external_str_new() ? assume always UTF-8?
    rb_raise(rb_eRuntimeError, "NEED TO IMPLEMNET STRING COPYING");

  return Qnil; // unreachable
}

static inline
CFStringRef
unwrap_string(VALUE string)
{
  return CFStringCreateWithCString(
				   NULL,
				   StringValuePtr(string),
				   kCFStringEncodingUTF8
				   );
}

static VALUE wrap_array_strings(CFArrayRef array) { WRAP_ARRAY(wrap_string) }


static inline
VALUE
wrap_long(CFNumberRef number)
{
  long value;
  if (CFNumberGetValue(number, kCFNumberLongType, &value))
    return LONG2FIX(value);
  rb_raise(rb_eRuntimeError, "I goofed on wrapping a long!");
  return Qnil;
}

static inline
CFNumberRef
unwrap_long(VALUE num)
{
  long base = NUM2LONG(num);
  return CFNumberCreate(NULL, kCFNumberLongType, &base);
}

static inline
VALUE
wrap_long_long(CFNumberRef number)
{
  long long value;
  if (CFNumberGetValue(number, kCFNumberLongLongType, &value))
    return LL2NUM(value);
  rb_raise(rb_eRuntimeError, "I goofed on wrapping a long long!");
  return Qnil;
}

static inline
CFNumberRef
unwrap_long_long(VALUE num)
{
  long long base = NUM2LL(num);
  return CFNumberCreate(NULL, kCFNumberLongLongType, &base);
}

static inline
VALUE
wrap_float(CFNumberRef number)
{
  double value;
  if (CFNumberGetValue(number, kCFNumberDoubleType, &value))
    return DBL2NUM(value);
  rb_raise(rb_eRuntimeError, "I goofed on wrapping a float!");
  return Qnil;
}

static inline
CFNumberRef
unwrap_float(VALUE num)
{
  int base = NUM2DBL(num);
  return CFNumberCreate(NULL, kCFNumberDoubleType, &base);
}

static
VALUE
wrap_number(CFNumberRef number)
{
  switch (CFNumberGetType(number))
    {
    case kCFNumberSInt8Type:
    case kCFNumberSInt16Type:
    case kCFNumberSInt32Type:
    case kCFNumberSInt64Type:
      return wrap_long(number);
    case kCFNumberFloat32Type:
    case kCFNumberFloat64Type:
      return wrap_float(number);
    case kCFNumberCharType:
    case kCFNumberShortType:
    case kCFNumberIntType:
    case kCFNumberLongType:
      return wrap_long(number);
    case kCFNumberLongLongType:
      return wrap_long_long(number);
    case kCFNumberFloatType:
    case kCFNumberDoubleType:
      return wrap_float(number);
    case kCFNumberCFIndexType:
    case kCFNumberNSIntegerType:
      return wrap_long(number);
    case kCFNumberCGFloatType: // == kCFNumberMaxType
      return wrap_float(number);
    default:
      return INT2NUM(0); // unreachable unless system goofed
    }
}

static
CFNumberRef
unwrap_number(VALUE number)
{
  switch (TYPE(number))
    {
    case T_FIXNUM:
      return unwrap_long(number);
    case T_FLOAT:
      return unwrap_float(number);
    default:
      rb_raise(
	       rb_eRuntimeError,
	       "wrapping %s is not supported; log a bug?",
	       rb_string_value_cstr(&number)
	       );
      return kCFNumberNegativeInfinity; // unreachable
    }
}

static VALUE wrap_array_numbers(CFArrayRef array) { WRAP_ARRAY(wrap_number) }


static inline
VALUE
wrap_url(CFURLRef url)
{
#ifdef NOT_MACRUBY
  return rb_funcall(rb_mURI, sel_parse, 1, wrap_string(CFURLGetString(url)));
#else
  return (NSURL*)url;
#endif
}

static inline
CFURLRef
unwrap_url(VALUE url)
{
  CFStringRef string = CFStringCreateWithCString(
						 NULL,
						 StringValuePtr(url),
						 kCFStringEncodingUTF8
						 );
  CFURLRef url_ref = CFURLCreateWithString(NULL, string, NULL);
  RELEASE(string);
  return url_ref;
}

static VALUE wrap_array_urls(CFArrayRef array) { WRAP_ARRAY(wrap_url) }

static
VALUE
wrap_date(CFDateRef date)
{
#ifdef NOT_MACRUBY
  NSTimeInterval time = [(NSDate*)date timeIntervalSince1970];
  return rb_time_new((time_t)time, 0);
#else
  return (VALUE)date;
#endif
}

static
CFDateRef
unwrap_date(VALUE date)
{
  struct timeval t = rb_time_timeval(date);
  NSDate* ns_date = [NSDate dateWithTimeIntervalSince1970:t.tv_sec];
  return (CFDateRef)ns_date;
}

static VALUE wrap_array_dates(CFArrayRef array) { WRAP_ARRAY(wrap_date) }


static inline
VALUE
wrap_boolean(CFBooleanRef bool_val)
{
  return (CFBooleanGetValue(bool_val) ? Qtrue : Qfalse);
}

static inline
CFBooleanRef
unwrap_boolean(VALUE bool_val)
{
  return (bool_val == Qtrue ? kCFBooleanTrue : kCFBooleanFalse);
}

static VALUE wrap_array_booleans(CFArrayRef array) { WRAP_ARRAY(wrap_boolean) }


static
VALUE
wrap_array(CFArrayRef array)
{
  CFTypeRef obj = CFArrayGetValueAtIndex(array, 0);
  CFTypeID di   = CFGetTypeID(obj);
       if (di == ref_type)     return wrap_array_refs(array);
  else if (di == value_type)   return wrap_array_values(array);
  else if (di == string_type)  return wrap_array_strings(array);
  else if (di == number_type)  return wrap_array_numbers(array);
  else if (di == boolean_type) return wrap_array_booleans(array);
  else if (di == url_type)     return wrap_array_urls(array);
  else if (di == date_type)    return wrap_array_dates(array);
  else {
    // for debugging, if we don't handle it give output to help log a bug
    CFShow(obj);
    return Qnil;
  }
}

static
VALUE
to_ruby(CFTypeRef obj)
{
  CFTypeID di = CFGetTypeID(obj);
  if      (di == array_type)   return wrap_array(obj);
  else if (di == ref_type)     return wrap_ref(obj);
  else if (di == value_type)   return wrap_value(obj);
  else if (di == string_type)  return wrap_string(obj);
  else if (di == number_type)  return wrap_number(obj);
  else if (di == boolean_type) return wrap_boolean(obj);
  else if (di == url_type)     return wrap_url(obj);
  else if (di == date_type)    return wrap_date(obj);
  else {
    // for debugging, if we don't handle it give output to help log a bug
    CFShow(obj);
    return Qnil;
  }
}

static
CFTypeRef
to_ax(VALUE obj)
{
  VALUE type = CLASS_OF(obj);
  if      (type == rb_cElement)            return unwrap_ref(obj);
  else if (type == rb_cString)             return unwrap_string(obj);
  else if (type == rb_cStruct)             return unwrap_value(obj);
  else if (type == rb_cFixnum)             return unwrap_number(obj);
  else if (type == rb_cTime)               return unwrap_date(obj);
  else if (type == rb_cURI)                return unwrap_url(obj);
  else if (obj  == Qtrue || obj == Qfalse) return unwrap_boolean(obj);
  else {
    // for debugging, if we don't handle it give output to help log a bug
    rb_raise(
	     rb_eRuntimeError,
	     "don't know how to convert %s objects :(",
	     rb_string_value_cstr(&type)
	     );
    return NULL;
  }
}


static
VALUE
handle_error(VALUE self, OSStatus code)
{
  // TODO port the error handler from AXElements
  rb_raise(rb_eRuntimeError, "you done goofed [%d]", code);
  return Qnil;
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
  NSDate* date = [NSDate date];
  [[NSRunLoop currentRunLoop] runUntilDate:date];
  RELEASE(date);

  pid_t                     the_pid = NUM2PIDT(pid);
  NSRunningApplication* running_app =
    [NSRunningApplication runningApplicationWithProcessIdentifier:the_pid];

  if (running_app) {
    VALUE app = wrap_ref(AXUIElementCreateApplication(the_pid));
    RELEASE(running_app);
    return app;
  }

  rb_raise(
	   rb_eArgError,
	   "pid `%d' must belong to a running application",
	   the_pid
	   );

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
  return wrap_ref(AXUIElementCreateSystemWide());
}

static inline
int
acore_is_system_wide(VALUE other)
{
  AXUIElementRef system = AXUIElementCreateSystemWide();
  int result = CFEqual(unwrap_ref(other), system);
  RELEASE(system);
  return result;
}
#define IS_SYSTEM_WIDE(x) (acore_is_system_wide(x))


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
  VALUE cached_attrs = rb_ivar_get(self, ivar_attrs);
  if (cached_attrs != Qnil)
    return cached_attrs;

  CFArrayRef attrs = NULL;
  OSStatus    code = AXUIElementCopyAttributeNames(unwrap_ref(self), &attrs);
  switch (code)
    {
    case kAXErrorSuccess:
      cached_attrs = wrap_array_strings(attrs);
      rb_ivar_set(self, ivar_attrs, cached_attrs);
      RELEASE(attrs);
      return cached_attrs;
    case kAXErrorInvalidUIElement:
      return rb_ary_new();
    default:
      // TODO we should actually allow for a grace period and try again in
      //      every case where would be deferring to the default error handler,
      //      and maybe even in every case where we get a non-zero result code
      return handle_error(self, code);
    }
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
						unwrap_ref(self),
						attr_name,
						&attr
						);
  RELEASE(attr_name);
  switch (code)
    {
    case kAXErrorSuccess:
      return to_ruby(attr);
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}

/*
 * Get the list of parameterized attributes for the element
 *
 * @example
 *
 *   button.parameterized_attributes # => ["AXTextForRange", ...]
 *
 * @return [Array<String>]
 */
static
VALUE
rb_acore_parameterized_attributes(VALUE self)
{
  VALUE cached_attrs = rb_ivar_get(self, ivar_param_attrs);
  if (cached_attrs != Qnil)
    return cached_attrs;

  CFArrayRef attrs = NULL;
  OSStatus    code = AXUIElementCopyParameterizedAttributeNames(
                                                                unwrap_ref(self),
								&attrs
								);
  switch (code)
    {
    case kAXErrorSuccess:
      cached_attrs = wrap_array_strings(attrs);
      rb_ivar_set(self, ivar_param_attrs, cached_attrs);
      RELEASE(attrs);
      return cached_attrs;
    case kAXErrorInvalidUIElement:
      return rb_ary_new();
    default:
      // TODO we should actually allow for a grace period and try again in
      //      every case where would be deferring to the default error handler,
      //      and maybe even in every case where we get a non-zero result code
      return handle_error(self, code);
    }
}


/*
 * Fetch the value for the given attribute and parameter
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
rb_acore_parameterized_attribute(VALUE self, VALUE name, VALUE parameter)
{
  CFTypeRef       param = to_ax(parameter);
  CFTypeRef        attr = NULL;
  CFStringRef attr_name = CFStringCreateWithCStringNoCopy(
							  NULL,
							  StringValueCStr(name),
							  0,
							  kCFAllocatorNull
							  );
  OSStatus code = AXUIElementCopyParameterizedAttributeValue(
						             unwrap_ref(self),
							     attr_name,
							     param,
							     &attr
							     );
  RELEASE(param);
  RELEASE(attr_name);
  switch (code)
    {
    case kAXErrorSuccess:
      return to_ruby(attr);
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


/*
 * Get the list of actions for the element
 *
 * As a convention, this method will return an empty array if the
 * backing element is no longer alive.
 *
 * @example
 *
 *   button.actions # => ["AXPress", ...]
 *
 * @return [Array<String>]
 */
static
VALUE
rb_acore_actions(VALUE self)
{
  VALUE cached_actions = rb_ivar_get(self, ivar_actions);
  if (cached_actions != Qnil)
    return cached_actions;

  CFArrayRef actions = NULL;
  OSStatus      code = AXUIElementCopyActionNames(unwrap_ref(self), &actions);
  switch (code)
    {
    case kAXErrorSuccess:
      cached_actions = wrap_array_strings(actions);
      rb_ivar_set(self, ivar_actions, cached_actions);
      RELEASE(actions);
      return cached_actions;
    case kAXErrorInvalidUIElement:
      return rb_ary_new();
    default:
      // TODO we should actually allow for a grace period and try again in
      //      every case where would be deferring to the default error handler,
      //      and maybe even in every case where we get a non-zero result code
      return handle_error(self, code);
    }
}


/*
 * Tell the receiver to perform the given action
 *
 * @example
 *   button.action "AXPress" # => true
 *
 * @param name [String]
 */
static
VALUE
rb_acore_perform_action(VALUE self, VALUE name)
{
  CFStringRef action = CFStringCreateWithCStringNoCopy(
		         			       NULL,
						       StringValueCStr(name),
						       0,
						       kCFAllocatorNull
						       );
  OSStatus code = AXUIElementPerformAction(unwrap_ref(self), action);

  RELEASE(action);
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
  OSStatus   code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXRoleAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      return wrap_string((CFStringRef)value);
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      return handle_error(self, code);
    }
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
  OSStatus   code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXSubroleAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      return wrap_string((CFStringRef)value);
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


/*
 * Shortcut for getting the `kAXParentAttribute`
 *
 * @example
 *   window.parent    # => "AXApplication"
 *   web_area.parent  # => "AXScrollArea"
 *
 * @return [Accessibility::Element,nil]
 */
static
VALUE
rb_acore_parent(VALUE self)
{
  CFTypeRef value = NULL;
  OSStatus   code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXParentAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      return wrap_ref(value);
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


/*
 * Shortcut for getting the `kAXChildrenAttribute`
 *
 * @example
 *   window.children    # => [AXButton, AXButton, AXScrollArea, AX...]
 *   button.children    # => []
 *
 * @return [Array<Accessibility::Element>]
 */
static
VALUE
rb_acore_children(VALUE self)
{
  CFTypeRef value = NULL;
  OSStatus   code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXChildrenAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      return wrap_array_refs(value);
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return rb_ary_new();
    default:
      return handle_error(self, code);
    }
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
  OSStatus code = AXUIElementGetPid(unwrap_ref(self), &pid);

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
  OSStatus code = AXUIElementSetMessagingTimeout(unwrap_ref(self), timeout);

  switch (code)
    {
    case kAXErrorSuccess:
      return seconds;
    default:
      return handle_error(self, code); // seconds
    }
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
  CGPoint          p = unwrap_point(point);
  OSStatus      code = AXUIElementCopyElementAtPosition(
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
      return handle_error(self, code); // point, nil, nil
    }
}


void
Init_core()
{
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

  array_type   = CFArrayGetTypeID();
  ref_type     = AXUIElementGetTypeID();
  value_type   = AXValueGetTypeID();
  string_type  = CFStringGetTypeID();
  number_type  = CFNumberGetTypeID();
  boolean_type = CFBooleanGetTypeID();
  url_type     = CFURLGetTypeID();
  date_type    = CFDateGetTypeID();

  sel_new      = rb_intern("new");
  sel_x        = rb_intern("x");
  sel_y        = rb_intern("y");
  sel_width    = rb_intern("width");
  sel_height   = rb_intern("height");
  sel_origin   = rb_intern("origin");
  sel_size     = rb_intern("size");
  sel_to_point = rb_intern("to_point");
  sel_to_size  = rb_intern("to_size");
  sel_to_rect  = rb_intern("to_rect");
  sel_to_range = rb_intern("to_range");

  ivar_attrs       = rb_intern("@attrs");
  ivar_param_attrs = rb_intern("@param_attrs");
  ivar_actions     = rb_intern("@actions");
  ivar_pid         = rb_intern("@pid");

  // on either supported Ruby, these should be defined by now
  rb_cCGPoint       = rb_const_get(rb_cObject, rb_intern("CGPoint"));
  rb_cCGSize        = rb_const_get(rb_cObject, rb_intern("CGSize"));
  rb_cCGRect        = rb_const_get(rb_cObject, rb_intern("CGRect"));
  rb_mURI           = rb_const_get(rb_cObject, rb_intern("URI"));
  rb_cURI           = rb_const_get(rb_mURI,    rb_intern("Generic"));
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
   *   element = Accessibility::Element.application_for 277 # pid of Terminal.app
   *   element.attributes                      # => ["AXRole", "AXMainWindow", ...]
   *   element.attribute "AXMainWindow"        # => #<Accessibility::Element...>
   */
  rb_cElement = rb_define_class_under(rb_mAccessibility, "Element", rb_cObject);

  rb_define_singleton_method(rb_cElement, "application_for", rb_acore_application_for,          1);
  rb_define_singleton_method(rb_cElement, "system_wide",     rb_acore_system_wide,              0);

  rb_define_method(rb_cElement, "attributes",                rb_acore_attributes,               0);
  rb_define_method(rb_cElement, "attribute",                 rb_acore_attribute,                1);
  rb_define_method(rb_cElement, "parameterized_attributes",  rb_acore_parameterized_attributes, 0);
  rb_define_method(rb_cElement, "parameterized_attribute",   rb_acore_parameterized_attribute,  2);
  rb_define_method(rb_cElement, "actions",                   rb_acore_actions,                  0);
  rb_define_method(rb_cElement, "perform_action",            rb_acore_perform_action,           1);

  rb_define_method(rb_cElement, "role",                      rb_acore_role,                     0);
  rb_define_method(rb_cElement, "subrole",                   rb_acore_subrole,                  0);
  rb_define_method(rb_cElement, "parent",                    rb_acore_parent,                   0);
  rb_define_method(rb_cElement, "children",                  rb_acore_children,                 0);

  rb_define_method(rb_cElement, "pid",                       rb_acore_pid,                      0);
  rb_define_method(rb_cElement, "set_timeout_to",            rb_acore_set_timeout_to,           1);
  rb_define_method(rb_cElement, "application",               rb_acore_application,              0);
  rb_define_method(rb_cElement, "element_at",                rb_acore_element_at,               1);
}
