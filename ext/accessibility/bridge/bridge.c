#include "ruby.h"
#include "ruby/encoding.h"
#import <Cocoa/Cocoa.h>

#ifndef ACCESSIBILITY_BRIDGE
#define ACCESSIBILITY_BRIDGE 1

#ifdef NOT_MACRUBY

VALUE rb_mAccessibility;
VALUE rb_cElement;
VALUE rb_cCGPoint;
VALUE rb_cCGSize;
VALUE rb_cCGRect;
VALUE rb_mURI; // URI module
VALUE rb_cURI; // URI::Generic class


ID sel_x;
ID sel_y;
ID sel_width;
ID sel_height;
ID sel_origin;
ID sel_size;
ID sel_to_point;
ID sel_to_size;
ID sel_to_rect;
ID sel_to_s;
ID sel_parse;


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


VALUE
wrap_point(CGPoint point)
{
  return rb_struct_new(rb_cCGPoint, DBL2NUM(point.x), DBL2NUM(point.y));
}

CGPoint
unwrap_point(VALUE point)
{
  point = rb_funcall(point, sel_to_point, 0);
  double x = NUM2DBL(rb_struct_getmember(point, sel_x));
  double y = NUM2DBL(rb_struct_getmember(point, sel_y));
  return CGPointMake(x, y);
}


VALUE
wrap_size(CGSize size)
{
  return rb_struct_new(rb_cCGSize, DBL2NUM(size.width), DBL2NUM(size.height));
}

CGSize
unwrap_size(VALUE size)
{
  size = rb_funcall(size, sel_to_size, 0);
  double width  = NUM2DBL(rb_struct_getmember(size, sel_width));
  double height = NUM2DBL(rb_struct_getmember(size, sel_height));
  return CGSizeMake(width, height);
}


VALUE
wrap_rect(CGRect rect)
{
  VALUE point = wrap_point(rect.origin);
  VALUE  size = wrap_size(rect.size);
  return rb_struct_new(rb_cCGRect, point, size);
}

CGRect
unwrap_rect(VALUE rect)
{
  rect = rb_funcall(rect, sel_to_rect, 0);
  CGPoint origin = unwrap_point(rb_struct_getmember(rect, sel_origin));
  CGSize    size = unwrap_size(rb_struct_getmember(rect, sel_size));
  return CGRectMake(origin.x, origin.y, size.width, size.height);
}


VALUE
convert_cf_range(CFRange range)
{
  CFIndex end_index = range.location + range.length;
  if (range.length != 0)
    end_index -= 1;
  return rb_range_new(INT2FIX(range.location), INT2FIX(end_index), 0);
}

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

VALUE wrap_value_point(AXValueRef value) { WRAP_VALUE(CGPoint,  kAXValueCGPointType, wrap_point) }
VALUE wrap_value_size(AXValueRef value)  { WRAP_VALUE(CGSize,   kAXValueCGSizeType,  wrap_size)  }
VALUE wrap_value_rect(AXValueRef value)  { WRAP_VALUE(CGRect,   kAXValueCGRectType,  wrap_rect)  }
VALUE wrap_value_range(AXValueRef value) { WRAP_VALUE(CFRange,  kAXValueCFRangeType, convert_cf_range) }
VALUE wrap_value_error(AXValueRef value) { WRAP_VALUE(AXError,  kAXValueAXErrorType, INT2NUM)    }

#define UNWRAP_VALUE(type, value, unwrapper) do {		\
    type st = unwrapper(val);					\
    return AXValueCreate(value, &st);				\
  }  while(0);

AXValueRef unwrap_value_point(VALUE val) { UNWRAP_VALUE(CGPoint, kAXValueCGPointType, unwrap_point) }
AXValueRef unwrap_value_size(VALUE val)  { UNWRAP_VALUE(CGSize,  kAXValueCGSizeType,  unwrap_size)  }
AXValueRef unwrap_value_rect(VALUE val)  { UNWRAP_VALUE(CGRect,  kAXValueCGRectType,  unwrap_rect)  }
AXValueRef unwrap_value_range(VALUE val) { UNWRAP_VALUE(CFRange, kAXValueCFRangeType, convert_rb_range) }


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

VALUE wrap_array_values(CFArrayRef array) { WRAP_ARRAY(wrap_value) }


static
void
ref_finalizer(void* obj)
{
  CFRelease((CFTypeRef)obj);
}

VALUE
wrap_ref(AXUIElementRef ref)
{
  return Data_Wrap_Struct(rb_cElement, NULL, ref_finalizer, (void*)ref);
}

AXUIElementRef
unwrap_ref(VALUE obj)
{
  AXUIElementRef* ref;
  Data_Get_Struct(obj, AXUIElementRef, ref);
  // TODO we should return *ref? but that seems to fuck things up...
  return (AXUIElementRef)ref;
}

VALUE wrap_array_refs(CFArrayRef array) { WRAP_ARRAY(wrap_ref) }


VALUE
wrap_string(CFStringRef string)
{
  // flying by the seat of our pants here, this hasn't failed yet
  // but probably will one day when I'm not looking
  VALUE ruby_string;
  CFIndex    length = CFStringGetLength(string);
  char*        name = (char*)CFStringGetCStringPtr(string, kCFStringEncodingMacRoman);

  if (name) {
    ruby_string = rb_str_new(name, length);
  }
  else {
    // currently we will always assume UTF-8
    // perhaps we could use CFStringGetSystemEncoding in the future?
    name = malloc(length+1);
    CFStringGetCString(
		       string,
		       name,
		       length+1,
		       kCFStringEncodingUTF8
		       );
    ruby_string = rb_enc_str_new(name, length, rb_utf8_encoding());
    free(name);
  }

  return ruby_string;
}

inline
VALUE
wrap_nsstring(NSString* string)
{
  return wrap_string((CFStringRef)string);
}


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

inline
NSString*
unwrap_nsstring(VALUE string)
{
  return (NSString*)unwrap_string(string);
}

VALUE wrap_array_strings(CFArrayRef array) { WRAP_ARRAY(wrap_string); }
VALUE wrap_array_nsstrings(NSArray* ary)
{
  CFArrayRef array = (CFArrayRef)ary;
  WRAP_ARRAY(wrap_string);
}


#define WRAP_NUM(type, cookie, macro) do {		        \
    type value;							\
    if (CFNumberGetValue(num, cookie, &value))			\
      return macro(value);					\
    rb_raise(rb_eRuntimeError, "I goofed wrapping a number");	\
    return Qnil;						\
  } while(0);

VALUE wrap_long(CFNumberRef num)      { WRAP_NUM(long,      kCFNumberLongType,     LONG2FIX) }
VALUE wrap_long_long(CFNumberRef num) { WRAP_NUM(long long, kCFNumberLongLongType, LL2NUM)   }
VALUE wrap_float(CFNumberRef num)     { WRAP_NUM(double,    kCFNumberDoubleType,   DBL2NUM)  }

#define UNWRAP_NUM(type, cookie, macro) do {	\
    type base = macro(num);			\
    return CFNumberCreate(NULL, cookie, &base); \
  } while(0);

CFNumberRef unwrap_long(VALUE num)      { UNWRAP_NUM(long,      kCFNumberLongType,     NUM2LONG) }
CFNumberRef unwrap_long_long(VALUE num) { UNWRAP_NUM(long long, kCFNumberLongLongType, NUM2LL)   }
CFNumberRef unwrap_float(VALUE num)     { UNWRAP_NUM(double,    kCFNumberDoubleType,   NUM2DBL)  }

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

VALUE wrap_array_numbers(CFArrayRef array) { WRAP_ARRAY(wrap_number) }


VALUE
wrap_url(CFURLRef url)
{
  // @note CFURLGetString does not need to be CFReleased since it is a Get
  return rb_funcall(rb_mURI, sel_parse, 1, wrap_string(CFURLGetString(url)));
}

inline
VALUE
wrap_nsurl(NSURL* url)
{
  return wrap_url((CFURLRef)url);
}

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

VALUE wrap_array_urls(CFArrayRef array) { WRAP_ARRAY(wrap_url) }

VALUE
wrap_date(CFDateRef date)
{
  NSTimeInterval time = [(NSDate*)date timeIntervalSince1970];
  return rb_time_new((time_t)time, 0);
}

inline
VALUE
wrap_nsdate(NSDate* date)
{
  return wrap_date((CFDateRef)date);
}

CFDateRef
unwrap_date(VALUE date)
{
  struct timeval t = rb_time_timeval(date);
  NSDate* ns_date = [NSDate dateWithTimeIntervalSince1970:t.tv_sec];
  return (CFDateRef)ns_date;
}

VALUE wrap_array_dates(CFArrayRef array) { WRAP_ARRAY(wrap_date) }


VALUE
wrap_boolean(CFBooleanRef bool_val)
{
  return (CFBooleanGetValue(bool_val) ? Qtrue : Qfalse);
}

CFBooleanRef
unwrap_boolean(VALUE bool_val)
{
  return (bool_val == Qtrue ? kCFBooleanTrue : kCFBooleanFalse);
}

VALUE wrap_array_booleans(CFArrayRef array) { WRAP_ARRAY(wrap_boolean) }


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

CFTypeRef
to_ax(VALUE obj)
{
  // TODO we can better optimize this when running under MacRuby
  VALUE type = CLASS_OF(obj);
  if      (type == rb_cElement)            return unwrap_ref(obj);
  else if (type == rb_cString)             return unwrap_string(obj);
  else if (type == rb_cFixnum)             return unwrap_number(obj);
  else if (type == rb_cCGPoint)            return unwrap_value(obj);
  else if (type == rb_cCGSize)             return unwrap_value(obj);
  else if (type == rb_cCGRect)             return unwrap_value(obj);
  else if (type == rb_cRange)              return unwrap_value(obj);
  else if (type == rb_cFloat)              return unwrap_number(obj);
  else if (type == rb_cTime)               return unwrap_date(obj);
  else if (type == rb_cURI)                return unwrap_url(obj);
  else if (obj  == Qtrue || obj == Qfalse) return unwrap_boolean(obj);
  else                                     return unwrap_unknown(obj);
}

#endif


void
Init_bridge()
{
#ifdef NOT_MACRUBY
  sel_x        = rb_intern("x");
  sel_y        = rb_intern("y");
  sel_width    = rb_intern("width");
  sel_height   = rb_intern("height");
  sel_origin   = rb_intern("origin");
  sel_size     = rb_intern("size");
  sel_to_point = rb_intern("to_point");
  sel_to_size  = rb_intern("to_size");
  sel_to_rect  = rb_intern("to_rect");
  sel_to_s     = rb_intern("to_s");
  sel_parse    = rb_intern("parse");

  rb_cCGPoint = rb_const_get(rb_cObject, rb_intern("CGPoint"));
  rb_cCGSize  = rb_const_get(rb_cObject, rb_intern("CGSize"));
  rb_cCGRect  = rb_const_get(rb_cObject, rb_intern("CGRect"));
  rb_mURI     = rb_const_get(rb_cObject, rb_intern("URI"));
  rb_cURI     = rb_const_get(rb_mURI,    rb_intern("Generic"));
#endif
}

#endif
