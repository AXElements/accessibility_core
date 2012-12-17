#include "ruby.h"
#ifdef NOT_MACRUBY /* This entire extension is pointless when running on MacRuby */

#import <Cocoa/Cocoa.h>

static VALUE rb_mAccessibility;
static VALUE rb_cElement;
static VALUE rb_cCGPoint;
static VALUE rb_cCGSize;
static VALUE rb_cCGRect;
static VALUE rb_mURI; // URI module
static VALUE rb_cURI; // URI::Generic class

static ID ivar_attrs;
static ID ivar_param_attrs;
static ID ivar_actions;
static ID ivar_pid;
static ID ivar_key_rate;

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
static ID sel_to_s;
static ID sel_to_f;
static ID sel_parse;

static ID rate_very_slow;
static ID rate_slow;
static ID rate_normal;
static ID rate_default;
static ID rate_fast;
static ID rate_zomg;


#define WRAP_ARRAY(wrapper) do {				\
    CFIndex length = CFArrayGetCount(array);			\
    VALUE      ary = rb_ary_new2(length);			\
								\
    for (CFIndex idx = 0; idx < length; idx++)			\
      rb_ary_store(						\
		   ary,						\
		   idx,						\
		   wrapper(CFArrayGetValueAtIndex(array, idx))	\
		   );	                                        \
    return ary;							\
  } while (false);


static
VALUE
wrap_unknown(CFTypeRef obj)
{
  // TODO: this will leak, can we use something like alloca?
  CFStringRef description = CFCopyDescription(obj);
  rb_raise(
           rb_eRuntimeError,
	     "accessibility-core doesn't know how to wrap `%s` objects yet",
	   CFStringGetCStringPtr(description, kCFStringEncodingMacRoman)
	   );
  return Qnil; // unreachable
}

static
CFTypeRef
unwrap_unknown(VALUE obj)
{
  obj = rb_funcall(obj, sel_to_s, 0);
  rb_raise(
	   rb_eRuntimeError,
	   "accessibility-core doesn't know how to unwrap `%s'",
	   StringValuePtr(obj)
	   );
  return NULL; // unreachable
}


static inline
VALUE
wrap_point(CGPoint point)
{
  return rb_struct_new(rb_cCGPoint, DBL2NUM(point.x), DBL2NUM(point.y));
}

static
CGPoint
unwrap_point(VALUE point)
{
  point = rb_funcall(point, sel_to_point, 0);
  double x = NUM2DBL(rb_struct_getmember(point, sel_x));
  double y = NUM2DBL(rb_struct_getmember(point, sel_y));
  return CGPointMake(x, y);
}


static inline
VALUE
wrap_size(CGSize size)
{
  return rb_struct_new(rb_cCGSize, DBL2NUM(size.width), DBL2NUM(size.height));
}

static
CGSize
unwrap_size(VALUE size)
{
  size = rb_funcall(size, sel_to_size, 0);
  double width  = NUM2DBL(rb_struct_getmember(size, sel_width));
  double height = NUM2DBL(rb_struct_getmember(size, sel_height));
  return CGSizeMake(width, height);
}


static inline
VALUE
wrap_rect(CGRect rect)
{
  VALUE point = wrap_point(rect.origin);
  VALUE  size = wrap_size(rect.size);
  return rb_struct_new(rb_cCGRect, point, size);
}

static
CGRect
unwrap_rect(VALUE rect)
{
  rect = rb_funcall(rect, sel_to_rect, 0);
  CGPoint origin = unwrap_point(rb_struct_getmember(rect, sel_origin));
  CGSize    size = unwrap_size(rb_struct_getmember(rect, sel_size));
  return CGRectMake(origin.x, origin.y, size.width, size.height);
}


static inline
VALUE
convert_cf_range(CFRange range)
{
  CFIndex end_index = range.location + range.length;
  if (range.length != 0)
    end_index -= 1;
  return rb_range_new(INT2FIX(range.location), INT2FIX(end_index), 0);
}

static
CFRange
convert_rb_range(VALUE range)
{
  VALUE b, e;
  int exclusive;

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


#define WRAP_VALUE(type, cookie, wrapper) do {	\
    type st;					\
    AXValueGetValue(value, cookie, &st);	\
    return wrapper(st);				\
  } while (0);					\

static VALUE wrap_value_point(AXValueRef value) { WRAP_VALUE(CGPoint,  kAXValueCGPointType, wrap_point) }
static VALUE wrap_value_size(AXValueRef value)  { WRAP_VALUE(CGSize,   kAXValueCGSizeType,  wrap_size)  }
static VALUE wrap_value_rect(AXValueRef value)  { WRAP_VALUE(CGRect,   kAXValueCGRectType,  wrap_rect)  }
static VALUE wrap_value_range(AXValueRef value) { WRAP_VALUE(CFRange,  kAXValueCFRangeType, convert_cf_range) }
static VALUE wrap_value_error(AXValueRef value) { WRAP_VALUE(AXError,  kAXValueAXErrorType, INT2NUM)    }

#define UNWRAP_VALUE(type, value, unwrapper) do {		\
    type st = unwrapper(val);					\
    return AXValueCreate(value, &st);				\
  }  while(0);

static AXValueRef unwrap_value_point(VALUE val) { UNWRAP_VALUE(CGPoint, kAXValueCGPointType, unwrap_point) }
static AXValueRef unwrap_value_size(VALUE val)  { UNWRAP_VALUE(CGSize,  kAXValueCGSizeType,  unwrap_size)  }
static AXValueRef unwrap_value_rect(VALUE val)  { UNWRAP_VALUE(CGRect,  kAXValueCGRectType,  unwrap_rect)  }
static AXValueRef unwrap_value_range(VALUE val) { UNWRAP_VALUE(CFRange, kAXValueCFRangeType, convert_rb_range) }


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
      // TODO better error message
      rb_raise(
	       rb_eRuntimeError,
	       "Could not wrap You've found a bug in something...not sure who to blame"
	       );
    }

  return Qnil; // unreachable
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


static
void
ref_finalizer(void* obj)
{
  CFRelease((CFTypeRef)obj);
}

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
  // TODO we should return *ref? but that seems to fuck things up...
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
  return CFStringCreateWithCStringNoCopy(
					 NULL,
					 StringValueCStr(string),
					 0,
					 kCFAllocatorNull
					 );
  /* return CFStringCreateWithCString( */
  /* 				   NULL, */
  /* 				   StringValuePtr(string), */
  /* 				   kCFStringEncodingUTF8 */
  /* 				   ); */
}

static VALUE wrap_array_strings(CFArrayRef array) { WRAP_ARRAY(wrap_string) }


#define WRAP_NUM(type, cookie, macro) do {		        \
    type value;							\
    if (CFNumberGetValue(num, cookie, &value))			\
      return macro(value);					\
    rb_raise(rb_eRuntimeError, "I goofed wrapping a number");	\
    return Qnil;						\
  } while(0);

static inline VALUE wrap_long(CFNumberRef num)      { WRAP_NUM(long,      kCFNumberLongType,     LONG2FIX) }
static inline VALUE wrap_long_long(CFNumberRef num) { WRAP_NUM(long long, kCFNumberLongLongType, LL2NUM)   }
static inline VALUE wrap_float(CFNumberRef num)     { WRAP_NUM(double,    kCFNumberDoubleType,   DBL2NUM)  }

#define UNWRAP_NUM(type, cookie, macro) do {	\
    type base = macro(num);			\
    return CFNumberCreate(NULL, cookie, &base); \
  } while(0);

static inline CFNumberRef unwrap_long(VALUE num)      { UNWRAP_NUM(long,      kCFNumberLongType,     NUM2LONG) }
static inline CFNumberRef unwrap_long_long(VALUE num) { UNWRAP_NUM(long long, kCFNumberLongLongType, NUM2LL)   }
static inline CFNumberRef unwrap_float(VALUE num)     { UNWRAP_NUM(double,    kCFNumberDoubleType,   NUM2DBL)  }

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
  return rb_funcall(rb_mURI, sel_parse, 1, wrap_string(CFURLGetString(url)));
}

static inline
CFURLRef
unwrap_url(VALUE url)
{
  url = rb_funcall(url, sel_to_s, 0);
  CFStringRef string = CFStringCreateWithCString(
						 NULL,
						 StringValuePtr(url),
						 kCFStringEncodingUTF8
						 );
  CFURLRef url_ref = CFURLCreateWithString(NULL, string, NULL);
  CFRelease(string);
  return url_ref;
}

static VALUE wrap_array_urls(CFArrayRef array) { WRAP_ARRAY(wrap_url) }

static
VALUE
wrap_date(CFDateRef date)
{
  NSTimeInterval time = [(NSDate*)date timeIntervalSince1970];
  return rb_time_new((time_t)time, 0);
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
  CFTypeID   di = CFGetTypeID(obj);
  if      (di == AXUIElementGetTypeID()) return wrap_array_refs(array);
  else if (di == AXValueGetTypeID())     return wrap_array_values(array);
  else if (di == CFStringGetTypeID())    return wrap_array_strings(array);
  else if (di == CFNumberGetTypeID())    return wrap_array_numbers(array);
  else if (di == CFBooleanGetTypeID())   return wrap_array_booleans(array);
  else if (di == CFURLGetTypeID())       return wrap_array_urls(array);
  else if (di == CFDateGetTypeID())      return wrap_array_dates(array);
  else                                   return wrap_unknown(obj);
}

static
VALUE
to_ruby(CFTypeRef obj)
{
  CFTypeID di = CFGetTypeID(obj);
  if      (di == CFArrayGetTypeID())     return wrap_array(obj);
  else if (di == AXUIElementGetTypeID()) return wrap_ref(obj);
  else if (di == AXValueGetTypeID())     return wrap_value(obj);
  else if (di == CFStringGetTypeID())    return wrap_string(obj);
  else if (di == CFNumberGetTypeID())    return wrap_number(obj);
  else if (di == CFBooleanGetTypeID())   return wrap_boolean(obj);
  else if (di == CFURLGetTypeID())       return wrap_url(obj);
  else if (di == CFDateGetTypeID())      return wrap_date(obj);
  else                                   return wrap_unknown(obj);
}

static
CFTypeRef
to_ax(VALUE obj)
{
  // TODO we can better optimize this when running under MacRuby
  VALUE type = CLASS_OF(obj);
  if      (type == rb_cElement)            return unwrap_ref(obj);
  else if (type == rb_cString)             return unwrap_string(obj);
  else if (type == rb_cStruct)             return unwrap_value(obj);
  else if (type == rb_cRange)              return unwrap_value(obj);
  else if (type == rb_cFixnum)             return unwrap_number(obj);
  else if (type == rb_cFloat)              return unwrap_number(obj);
  else if (type == rb_cTime)               return unwrap_date(obj);
  else if (type == rb_cURI)                return unwrap_url(obj);
  else if (obj  == Qtrue || obj == Qfalse) return unwrap_boolean(obj);
  else                                     return unwrap_unknown(obj);
}


static
VALUE
handle_error(VALUE self, AXError code)
{
  // TODO port the error handler from AXElements
  switch (code)
    {
    case kAXErrorInvalidUIElement:
      rb_raise(rb_eArgError, "invalid element (probably dead)");
    case kAXErrorAttributeUnsupported:
      rb_raise(rb_eArgError, "attribute unsupported");
    default:
      rb_raise(rb_eRuntimeError, "you done goofed [%d]", code);
    }

  return Qnil;
}


static
VALUE
rb_acore_application_for(VALUE self, VALUE pid)
{
  NSDate* date = [NSDate date];
  [[NSRunLoop currentRunLoop] runUntilDate:date];
  CFRelease(date);

  pid_t                     the_pid = NUM2PIDT(pid);
  NSRunningApplication* running_app =
    [NSRunningApplication runningApplicationWithProcessIdentifier:the_pid];

  if (running_app) {
    VALUE app = wrap_ref(AXUIElementCreateApplication(the_pid));
    CFRelease(running_app);
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
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_attribute(VALUE self, VALUE name)
{
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
      return to_ruby(attr);
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
      // TODO we should actually allow for a grace period and try again in
      //      every case where would be deferring to the default error handler,
      //      and maybe even in every case where we get a non-zero result code
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_parameterized_attribute(VALUE self, VALUE name, VALUE parameter)
{
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
      return to_ruby(attr);
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
      // TODO we should actually allow for a grace period and try again in
      //      every case where would be deferring to the default error handler,
      //      and maybe even in every case where we get a non-zero result code
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
}


static
VALUE
rb_acore_role(VALUE self)
{
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
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


static
VALUE
rb_acore_subrole(VALUE self)
{
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
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
    case kAXErrorNoValue:
    case kAXErrorInvalidUIElement:
      return Qnil;
    default:
      return handle_error(self, code);
    }
}


static
VALUE
rb_acore_children(VALUE self)
{
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
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


static
VALUE
rb_acore_value(VALUE self)
{
  CFTypeRef value = NULL;
  AXError    code = AXUIElementCopyAttributeValue(
						  unwrap_ref(self),
						  kAXValueAttribute,
						  &value
						  );
  switch (code)
    {
    case kAXErrorSuccess:
      return to_ruby(value);
    default:
      return handle_error(self, code);
    }
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
  CFRelease(value);
  return (code ? Qfalse : Qtrue);
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
      return handle_error(self, code); // point, nil, nil
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
#endif


void
Init_core()
{
#ifdef NOT_MACRUBY
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
  sel_to_s     = rb_intern("to_s");
  sel_to_f     = rb_intern("to_f");
  sel_parse    = rb_intern("parse");

  ivar_attrs       = rb_intern("@attrs");
  ivar_param_attrs = rb_intern("@param_attrs");
  ivar_actions     = rb_intern("@actions");
  ivar_pid         = rb_intern("@pid");
  ivar_key_rate    = rb_intern("@key_rate");

  rate_very_slow = rb_intern("very_slow");
  rate_slow      = rb_intern("slow");
  rate_normal    = rb_intern("normal");
  rate_default   = rb_intern("default");
  rate_fast      = rb_intern("fast");
  rate_zomg      = rb_intern("zomg");

  // these should be defined by now
  rb_cCGPoint       = rb_const_get(rb_cObject, rb_intern("CGPoint"));
  rb_cCGSize        = rb_const_get(rb_cObject, rb_intern("CGSize"));
  rb_cCGRect        = rb_const_get(rb_cObject, rb_intern("CGRect"));
  rb_mAccessibility = rb_const_get(rb_cObject, rb_intern("Accessibility"));
  rb_mURI           = rb_const_get(rb_cObject, rb_intern("URI"));
  rb_cURI           = rb_const_get(rb_mURI,    rb_intern("Generic"));


  rb_cElement = rb_define_class_under(rb_mAccessibility, "Element", rb_cObject);

  rb_define_singleton_method(rb_cElement, "application_for", rb_acore_application_for,          1);
  rb_define_singleton_method(rb_cElement, "system_wide",     rb_acore_system_wide,              0);
  rb_define_singleton_method(rb_cElement, "key_rate",        rb_acore_key_rate,                 0);
  rb_define_singleton_method(rb_cElement, "key_rate=",       rb_acore_set_key_rate,             1);
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

  rb_define_method(rb_cElement, "parameterized_attributes",  rb_acore_parameterized_attributes, 0);
  rb_define_method(rb_cElement, "parameterized_attribute",   rb_acore_parameterized_attribute,  2);

  rb_define_method(rb_cElement, "actions",                   rb_acore_actions,                  0);
  rb_define_method(rb_cElement, "perform",                   rb_acore_perform,                  1);
  rb_define_method(rb_cElement, "post",                      rb_acore_post,                     1);

  rb_define_method(rb_cElement, "invalid?",                  rb_acore_is_invalid,               0);
  rb_define_method(rb_cElement, "pid",                       rb_acore_pid,                      0);
  rb_define_method(rb_cElement, "set_timeout_to",            rb_acore_set_timeout_to,           1);
  // TODO make this meaningful, currently has no effect on calling rb_acore_post
  rb_define_method(rb_cElement, "key_rate",                  rb_acore_key_rate,                 0);
  rb_define_method(rb_cElement, "key_rate=",                 rb_acore_set_key_rate,             1);
  rb_define_method(rb_cElement, "application",               rb_acore_application,              0);
  rb_define_method(rb_cElement, "element_at",                rb_acore_element_at,               1);
  rb_define_method(rb_cElement, "==",                        rb_acore_equality,                 1);

#endif
}
