#include "bridge.h"
#include "ruby/encoding.h"


void
spin(double seconds)
{
  NSDate* interval = [NSDate dateWithTimeIntervalSinceNow:seconds];
  [[NSRunLoop currentRunLoop] runUntilDate:interval];
  [interval release];
}


#ifdef NOT_MACRUBY

VALUE rb_cData;
VALUE rb_cAttributedString;
VALUE rb_mAccessibility;
VALUE rb_cElement;
VALUE rb_cCGPoint;
VALUE rb_cCGSize;
VALUE rb_cCGRect;
VALUE rb_mURI; // URI module
VALUE rb_cURI; // URI::Generic class
VALUE rb_cScreen;

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


void
cf_finalizer(void* obj)
{
  CFRelease((CFTypeRef)obj);
}

void
objc_finalizer(void* obj)
{
  [(id)obj release];
}


VALUE
wrap_unknown(CFTypeRef obj)
{
  // TODO: this will leak...
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
  // TODO: rb_check_convert_type instead?
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


VALUE
coerce_to_rect(VALUE obj)
{
  return rb_funcall(obj, sel_to_rect, 0);
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
      rb_raise(rb_eArgError, "cannot wrap %s objects", rb_class2name(CLASS_OF(value)));
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
	       "Either accessibility_core is out of date or your system has had a serious error"
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


VALUE wrap_ref(AXUIElementRef obj) { WRAP_OBJC(rb_cElement, cf_finalizer); }

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
  VALUE   rb_str = Qnil;
  CFDataRef data = CFStringCreateExternalRepresentation(
							NULL,
							string,
							kCFStringEncodingUTF8,
							0
							);
  if (data) {
    rb_str = rb_enc_str_new(
			    (char*)CFDataGetBytePtr(data),
			    CFDataGetLength(data),
			    rb_utf8_encoding()
			    );
    CFRelease(data);
  }
  else {
    CFRelease(data);
    CFShow(string);
    rb_raise(rb_eRuntimeError, "Could not convert a string to a Ruby string");
  }

  return rb_str;
}

VALUE
wrap_nsstring(NSString* string)
{
  return wrap_string((CFStringRef)string);
}

CFStringRef
unwrap_string(VALUE string)
{
  return CFStringCreateWithBytes(
                                 NULL,
                                 (UInt8*)StringValueCStr(string),
                                 RSTRING_LEN(string),
                                 kCFStringEncodingUTF8,
                                 false
                                 );
}

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


VALUE
wrap_attributed_string(CFAttributedStringRef string)
{
  return wrap_nsattributed_string((NSAttributedString*)string);
}

VALUE
wrap_nsattributed_string(NSAttributedString* obj)
{
  WRAP_OBJC(rb_cAttributedString, objc_finalizer);
}

CFAttributedStringRef
unwrap_attributed_string(VALUE string)
{
  return (CFAttributedStringRef)unwrap_nsattributed_string(string);
}

NSAttributedString*
unwrap_nsattributed_string(VALUE obj)
{
  UNWRAP_OBJC(NSAttributedString);
}

VALUE
wrap_array_attributed_strings(CFArrayRef array)
{
  WRAP_ARRAY(wrap_attributed_string);
}

VALUE wrap_array_nsattributed_strings(NSArray* ary)
{
  CFArrayRef array = (CFArrayRef)ary;
  WRAP_ARRAY(wrap_nsattributed_string);
}

static
VALUE
rb_astring_alloc(VALUE self)
{
  return wrap_nsattributed_string([NSAttributedString alloc]);
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

VALUE
wrap_nsurl(NSURL* url)
{
  NSString* str = [url absoluteString];
  VALUE  rb_str = wrap_nsstring(str);
  [str release];
  return rb_funcall(rb_mURI, sel_parse, 1, rb_str);
}

CFURLRef
unwrap_url(VALUE url)
{
  // TODO: should also force encoding to UTF-8 first?
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

NSURL*
unwrap_nsurl(VALUE url)
{
  return (NSURL*)unwrap_url(url);
}

VALUE wrap_array_urls(CFArrayRef array) { WRAP_ARRAY(wrap_url) }


VALUE
wrap_date(CFDateRef date)
{
  NSTimeInterval time = [(NSDate*)date timeIntervalSince1970];
  return rb_time_new((time_t)time, 0);
}

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
  CFIndex length = CFArrayGetCount(array);
  if (length) {
    CFTypeRef obj = CFArrayGetValueAtIndex(array, 0);
    CFTypeID   di = CFGetTypeID(obj);
    if      (di == AXUIElementGetTypeID())  return wrap_array_refs(array);
    else if (di == AXValueGetTypeID())      return wrap_array_values(array);
    else if (di == CFStringGetTypeID())     return wrap_array_strings(array);
    else if (di == CFNumberGetTypeID())     return wrap_array_numbers(array);
    else if (di == CFBooleanGetTypeID())    return wrap_array_booleans(array);
    else if (di == CFURLGetTypeID())        return wrap_array_urls(array);
    else if (di == CFDateGetTypeID())       return wrap_array_dates(array);
    else if (di == CFDictionaryGetTypeID()) return wrap_array_dictionaries(array);
    else                                    return wrap_unknown(obj);
  }
  return rb_ary_new();
}


VALUE
wrap_dictionary(NSDictionary* dict)
{
  __block VALUE hash = rb_hash_new();

  [dict enumerateKeysAndObjectsUsingBlock: ^(id key, id obj, BOOL* stop) {
      rb_hash_aset(hash, to_ruby(key), to_ruby(obj));
    }];

  return hash;
}

VALUE wrap_array_dictionaries(CFArrayRef array) { WRAP_ARRAY(wrap_dictionary); }


VALUE
to_ruby(CFTypeRef obj)
{
  CFTypeID di = CFGetTypeID(obj);
  if      (di == CFArrayGetTypeID())            return wrap_array(obj);
  else if (di == AXUIElementGetTypeID())        return wrap_ref(obj);
  else if (di == AXValueGetTypeID())            return wrap_value(obj);
  else if (di == CFStringGetTypeID())           return wrap_string(obj);
  else if (di == CFNumberGetTypeID())           return wrap_number(obj);
  else if (di == CFBooleanGetTypeID())          return wrap_boolean(obj);
  else if (di == CFURLGetTypeID())              return wrap_url(obj);
  else if (di == CFDateGetTypeID())             return wrap_date(obj);
  else if (di == CFDataGetTypeID())             return wrap_data(obj);
  else if (di == CFAttributedStringGetTypeID()) return wrap_attributed_string(obj);
  else if (di == CFDictionaryGetTypeID())       return wrap_dictionary(obj);
  else                                          return wrap_unknown(obj);
}

CFTypeRef
to_ax(VALUE obj)
{
  switch (TYPE(obj))
    {
    case T_STRING:
      return unwrap_string(obj);
    case T_FIXNUM:
      return unwrap_number(obj);
    case T_STRUCT:
      return unwrap_value(obj);
    case T_FLOAT:
      return unwrap_number(obj);
    case T_TRUE:
      return kCFBooleanTrue;
    case T_FALSE:
      return kCFBooleanFalse;
    }

  VALUE type = CLASS_OF(obj);
  if (type == rb_cTime)
    return unwrap_date(obj);
  else if (type == rb_cRange)
    return unwrap_value_range(obj);
  else if (type == rb_cAttributedString)
    return unwrap_attributed_string(obj);
  else if (type == rb_cData)
    return unwrap_data(obj);

  if (rb_obj_is_kind_of(obj, rb_cURI))
    return unwrap_url(obj);

  // give up if we get this far
  return unwrap_unknown(obj);
}


VALUE
wrap_data(CFDataRef data)
{
  return wrap_nsdata((NSData*)data);
}

VALUE
wrap_nsdata(NSData* obj)
{
  WRAP_OBJC(rb_cData, objc_finalizer);
}

VALUE wrap_array_data(CFArrayRef array) { WRAP_ARRAY(wrap_data); }
VALUE
wrap_array_nsdata(NSArray* ary)
{
  CFArrayRef array = (CFArrayRef)ary;
  WRAP_ARRAY(wrap_data);
}

CFDataRef
unwrap_data(VALUE data)
{
  return (CFDataRef)unwrap_nsdata(data);
}

NSData*
unwrap_nsdata(VALUE obj)
{
  UNWRAP_OBJC(NSData);
}


static
VALUE
rb_data_data(VALUE self)
{
  return wrap_nsdata([NSData data]);
}

static
VALUE
rb_data_with_contents_of_url(VALUE self, VALUE url)
{
  NSData* data = [NSData dataWithContentsOfURL:unwrap_nsurl(url)];
  if (data)
    return wrap_nsdata(data);
  return Qnil;
}

static
VALUE
rb_data_length(VALUE self)
{
  return ULONG2NUM([unwrap_nsdata(self) length]);
}

static
VALUE
rb_data_equality(VALUE self, VALUE other)
{
  OBJC_EQUALITY(rb_cData, unwrap_nsdata);
}

static
VALUE
rb_data_write_to_file(int argc, VALUE* argv, VALUE self)
{
  if (argc < 2)
    rb_raise(
	     rb_eArgError,
	     "wrong number of arguments, got %d, expected 2",
	     argc
	     );

  NSString* path = unwrap_nsstring(argv[0]);
  BOOL    result = [unwrap_nsdata(self) writeToFile:path
		                         atomically:(argv[1] == Qtrue)];

  [path release];

  return (result ? Qtrue : Qfalse);
}


static
VALUE
rb_data_to_str(VALUE self)
{
  NSData*      data = unwrap_nsdata(self);
  const void* bytes = [data bytes];
  NSUInteger length = [data length];
  return rb_enc_str_new(bytes, length, rb_ascii8bit_encoding());
}

static
VALUE
rb_str_to_data(VALUE self)
{
  NSData* data = [NSData dataWithBytes:(void*)StringValuePtr(self)
		                length:RSTRING_LEN(self)];
  if (data)
    return wrap_nsdata(data);
  return Qnil; // I don't think this is possible except in case of ENOMEM
}


static VALUE
rb_spin(int argc, VALUE* argv, VALUE self)
{
  if (argc == 0)
    spin(0);
  else
    spin(NUM2DBL(argv[0]));

  return self;
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

  rb_mAccessibility = rb_define_module("Accessibility");
  rb_cElement       = rb_define_class_under(rb_mAccessibility, "Element", rb_cObject);
  rb_cCGPoint       = rb_const_get(rb_cObject, rb_intern("CGPoint"));
  rb_cCGSize        = rb_const_get(rb_cObject, rb_intern("CGSize"));
  rb_cCGRect        = rb_const_get(rb_cObject, rb_intern("CGRect"));
  rb_mURI           = rb_const_get(rb_cObject, rb_intern("URI"));
  rb_cURI           = rb_const_get(rb_mURI, rb_intern("Generic"));


  /*
   * Document-class: NSAttributedString
   *
   * A 90% drop-in replacement for Cocoa's `NSAttributedString` class. The most likely
   * to use methods have been bridged. Remaining methods can be bridged upon request.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSAttributedString_Class/Reference/Reference.html)
   * for documentation on the methods available in this class.
   */
  rb_cAttributedString = rb_define_class("NSAttributedString", rb_cObject);

  // LOL, but required to be a drop-in replacement
  rb_define_singleton_method(rb_cAttributedString, "alloc", rb_astring_alloc, 0);

  // TODO: all these methods :(
  /* rb_define_method(rb_cAttributedString, "initWithString", rb_astring_init_with_string, -1); */
  /* rb_define_method(rb_cAttributedString, "initWithAttributedString", rb_astring_init_with_astring, 2); */
  /* rb_define_method(rb_cAttributedString, "string", rb_astring_string, 0); */
  /* rb_define_method(rb_cAttributedString, "length", rb_astring_length, 0); */
  /* rb_define_method(rb_cAttributedString, "attributesAtIndex", rb_astring_attrs_at_index, -1); */
  /* rb_define_method(rb_cAttributedString, "isEqualToAttributedString", rb_astring_equality, 1); */
  /* rb_define_method(rb_cAttributedString, "attributedSubstringFromRange", rb_astring_astring_from_range, 1); */

  /* rb_define_alias(rb_cAttributedString,  "string", "to_s"); */
  /* rb_define_alias(rb_cAttributedString,  "string", "to_str"); */
  /* rb_define_alias(rb_cAttributedString,  "isEqualToAttributedString", "=="); */


  /*
   * Document-class: NSData
   *
   * A 70% drop-in replacement for Cocoa's `NSData` class. Almost all
   * non-deprecated methods have been bridged.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSData_Class/Reference/Reference.html)
   * for documentation on the methods available in this class.
   */
  rb_cData = rb_define_class("NSData", rb_cObject);

  // TODO: implement commented out methods
  rb_define_singleton_method(rb_cData, "data", rb_data_data, 0);
  //rb_define_singleton_method(rb_cData, "dataWithBytes", rb_data_with_bytes, 2);
  //rb_define_singleton_method(rb_cData, "dataWithContentsOfFile", rb_data_with_contents_of_file, 1);
  rb_define_singleton_method(rb_cData, "dataWithContentsOfURL", rb_data_with_contents_of_url, 1);
  //rb_define_singleton_method(rb_cData, "dataWithData", rb_data_with_data, 1);

  //rb_define_method(rb_cData, "bytes",            rb_data_bytes, 0);
  //rb_define_method(rb_cData, "description",      rb_data_description, 0);
  //rb_define_method(rb_cData, "subdataWithRange", rb_data_subrange, 1);
  rb_define_method(rb_cData, "isEqualToData",    rb_data_equality, 1);
  rb_define_method(rb_cData, "length",           rb_data_length, 0);
  rb_define_method(rb_cData, "writeToFile",      rb_data_write_to_file, -1);
  //rb_define_method(rb_cData, "writeToURL",       rb_data_write_to_url, -1);
  rb_define_method(rb_cData, "to_str",           rb_data_to_str, 0);

  rb_define_alias(rb_cData,  "isEqualToData", "==");


  // misc freedom patches
  rb_define_method(rb_cString, "to_data", rb_str_to_data, 0);
  rb_define_method(rb_cObject, "spin", rb_spin, -1); // semi-private method
#endif
}
