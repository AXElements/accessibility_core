#include "bridge.h"
#include "ruby/encoding.h"
#include "assert.h"


void
spin(const double seconds)
{
  NSDate* const interval = [NSDate dateWithTimeIntervalSinceNow:seconds];
  [[NSRunLoop currentRunLoop] runUntilDate:interval];
  [interval release];
}


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


VALUE
wrap_unknown(CFTypeRef const obj)
{
    // TODO: this will leak...
    CFStringRef const description = CFCopyDescription(obj);

    rb_raise(rb_eRuntimeError,
             "accessibility-core doesn't know how to wrap `%s` objects yet",
             CFStringGetCStringPtr(description, kCFStringEncodingMacRoman));

    return Qnil; // unreachable
}

CFTypeRef
unwrap_unknown(const VALUE obj)
{
  // TODO: rb_check_convert_type instead?
  VALUE obj_string = rb_funcall(obj, sel_to_s, 0);
  rb_raise(rb_eRuntimeError,
	   "accessibility-core doesn't know how to unwrap `%s'",
	   StringValuePtr(obj_string));
  return NULL; // unreachable
}


VALUE
wrap_point(const CGPoint point)
{
  return rb_struct_new(rb_cCGPoint, DBL2NUM(point.x), DBL2NUM(point.y));
}

CGPoint
unwrap_point(const VALUE point)
{
    const  VALUE p = rb_funcall(point, sel_to_point, 0);
    const double x = NUM2DBL(rb_struct_getmember(p, sel_x));
    const double y = NUM2DBL(rb_struct_getmember(p, sel_y));
    return CGPointMake(x, y);
}


VALUE
wrap_size(const CGSize size)
{
  return rb_struct_new(rb_cCGSize, DBL2NUM(size.width), DBL2NUM(size.height));
}

CGSize
unwrap_size(const VALUE size)
{
    const  VALUE      s = rb_funcall(size, sel_to_size, 0);
    const double width  = NUM2DBL(rb_struct_getmember(s, sel_width));
    const double height = NUM2DBL(rb_struct_getmember(s, sel_height));
    return CGSizeMake(width, height);
}


VALUE
wrap_rect(const CGRect rect)
{
    const VALUE point = wrap_point(rect.origin);
    const VALUE  size = wrap_size(rect.size);
    return rb_struct_new(rb_cCGRect, point, size);
}


VALUE
coerce_to_rect(const VALUE obj)
{
  return rb_funcall(obj, sel_to_rect, 0);
}

CGRect
unwrap_rect(const VALUE rect)
{
    const   VALUE      r = rb_funcall(rect, sel_to_rect, 0);
    const CGPoint origin = unwrap_point(rb_struct_getmember(r, sel_origin));
    const CGSize    size = unwrap_size(rb_struct_getmember(r, sel_size));
    return CGRectMake(origin.x, origin.y, size.width, size.height);
}


VALUE
convert_cf_range(const CFRange range)
{
    const CFIndex end_index =
        range.location + (range.length ? range.length - 1 : 0);
    return rb_range_new(INT2FIX(range.location), INT2FIX(end_index), 0);
}

CFRange
convert_rb_range(const VALUE range)
{
    VALUE b, e;
    int exclusive;

    rb_range_values(range, &b, &e, &exclusive);

    const int begin = NUM2INT(b);
    const int   end = NUM2INT(e);

    if (begin < 0 || end < 0)
        // We don't know what the max length of the range will be, so we
        // can't count backwards.
        rb_raise(rb_eArgError,
                 "negative values are not allowed in ranges "
                 "that are converted to CFRange structures.");

    const int length = exclusive ? end-begin : end-begin + 1;
    return CFRangeMake(begin, length);
}


#define WRAP_VALUE(type, cookie, wrapper)                       \
    type st;                                                    \
    const Boolean result = AXValueGetValue(value, cookie, &st);	\
    assert(result);                                             \
    return wrapper(st);

VALUE wrap_value_point(AXValueRef const value) { WRAP_VALUE(CGPoint,  kAXValueCGPointType, wrap_point) }
VALUE wrap_value_size(AXValueRef  const value) { WRAP_VALUE(CGSize,   kAXValueCGSizeType,  wrap_size)  }
VALUE wrap_value_rect(AXValueRef  const value) { WRAP_VALUE(CGRect,   kAXValueCGRectType,  wrap_rect)  }
VALUE wrap_value_range(AXValueRef const value) { WRAP_VALUE(CFRange,  kAXValueCFRangeType, convert_cf_range) }
VALUE wrap_value_error(AXValueRef const value) { WRAP_VALUE(AXError,  kAXValueAXErrorType, INT2NUM)    }

#define UNWRAP_VALUE(type, value, unwrapper)     \
    const type st = unwrapper(val);              \
    return AXValueCreate(value, &st);

AXValueRef unwrap_value_point(VALUE val) { UNWRAP_VALUE(CGPoint, kAXValueCGPointType, unwrap_point) }
AXValueRef unwrap_value_size(VALUE val)  { UNWRAP_VALUE(CGSize,  kAXValueCGSizeType,  unwrap_size)  }
AXValueRef unwrap_value_rect(VALUE val)  { UNWRAP_VALUE(CGRect,  kAXValueCGRectType,  unwrap_rect)  }
AXValueRef unwrap_value_range(VALUE val) { UNWRAP_VALUE(CFRange, kAXValueCFRangeType, convert_rb_range) }


VALUE
wrap_value(AXValueRef const value)
{
    switch (AXValueGetType(value)) {
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
        rb_raise(rb_eRuntimeError,
                 "Either accessibility_core is out of date or your system "
                 "has had a serious error");
    }

    return Qnil; // unreachable
}

AXValueRef
unwrap_value(const VALUE value)
{
    const VALUE type = CLASS_OF(value);
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

VALUE wrap_array_values(CFArrayRef const array) { WRAP_ARRAY(wrap_value) }


VALUE wrap_ref(AXUIElementRef const obj) { WRAP_OBJC(rb_cElement, cf_finalizer); }

AXUIElementRef
unwrap_ref(const VALUE obj)
{
    AXUIElementRef* ref;
    Data_Get_Struct(obj, AXUIElementRef, ref);
    // TODO we should return *ref? but that seems to fuck things up...
    return (AXUIElementRef)ref;
}

VALUE wrap_array_refs(CFArrayRef const array) { WRAP_ARRAY(wrap_ref) }


VALUE
wrap_string(CFStringRef const string)
{
    CFDataRef const data =
        CFStringCreateExternalRepresentation(NULL,
                                             string,
                                             kCFStringEncodingUTF8,
                                             0);
    if (data) {
        const VALUE rb_str =
            rb_enc_str_new((char*)CFDataGetBytePtr(data),
                           CFDataGetLength(data),
                           rb_utf8_encoding());
        CFRelease(data);
        return rb_str;
    }

    CFRelease(data);
    CFShow(string); // uhh....why?
    rb_raise(rb_eRuntimeError, "Could not convert a string to a Ruby string");
    return Qnil;
}

VALUE
wrap_nsstring(NSString* const string)
{
  return wrap_string((CFStringRef)string);
}

CFStringRef
unwrap_string(const VALUE string)
{
    volatile VALUE str = string; // StringValue needs volatility guarantees :(
    return CFStringCreateWithBytes(NULL,
                                   (UInt8*)StringValueCStr(str),
                                   RSTRING_LEN(string),
                                   kCFStringEncodingUTF8,
                                   false);
}

NSString*
unwrap_nsstring(const VALUE string)
{
  return (NSString*)unwrap_string(string);
}

VALUE wrap_array_strings(CFArrayRef const array) { WRAP_ARRAY(wrap_string); }
VALUE wrap_array_nsstrings(NSArray* const ary)
{
  CFArrayRef const array = (CFArrayRef const)ary;
  WRAP_ARRAY(wrap_string);
}


VALUE
wrap_attributed_string(CFAttributedStringRef const string)
{
  return wrap_nsattributed_string((NSAttributedString* const)string);
}

VALUE
wrap_nsattributed_string(NSAttributedString* const obj)
{
  WRAP_OBJC(rb_cAttributedString, objc_finalizer);
}

CFAttributedStringRef
unwrap_attributed_string(const VALUE string)
{
  return (CFAttributedStringRef)unwrap_nsattributed_string(string);
}

NSAttributedString*
unwrap_nsattributed_string(const VALUE obj)
{
  UNWRAP_OBJC(NSAttributedString);
}

VALUE
wrap_array_attributed_strings(CFArrayRef const array)
{
  WRAP_ARRAY(wrap_attributed_string);
}

VALUE wrap_array_nsattributed_strings(NSArray* const ary)
{
  CFArrayRef const array = (CFArrayRef)ary;
  WRAP_ARRAY(wrap_nsattributed_string);
}

static
VALUE
rb_astring_alloc(const VALUE self)
{
  return wrap_nsattributed_string([NSAttributedString alloc]);
}

static
VALUE
rb_astring_init_with_string(const int argc, VALUE* const argv, const VALUE self)
{
    if (!argc)
        rb_raise(rb_eArgError, "wrong number of arguments (0 for 1+)");

    NSString* const              nsstring =
        unwrap_nsstring(argv[0]);
    NSAttributedString* const old_astring =
        unwrap_nsattributed_string(self);
    NSAttributedString* const new_astring =
        [old_astring initWithString:nsstring];
    [nsstring release];

    if (old_astring == new_astring)
        return self;
    return wrap_nsattributed_string(new_astring);
}

static
VALUE
rb_astring_string(const VALUE self)
{
    NSString* const string = [unwrap_nsattributed_string(self) string];
    const VALUE     rb_str = wrap_nsstring(string);
    return rb_str;
}

static
VALUE
rb_astring_length(const VALUE self)
{
  return ULONG2NUM([unwrap_nsattributed_string(self) length]);
}

static
VALUE
rb_astring_equality(const VALUE self, const VALUE other)
{
  OBJC_EQUALITY(rb_cAttributedString, unwrap_nsattributed_string);
}


#define WRAP_NUM(type, cookie, macro)                           \
    type value;							\
    if (CFNumberGetValue(num, cookie, &value))			\
        return macro(value);					\
    rb_raise(rb_eRuntimeError, "I goofed wrapping a number");	\
    return Qnil;

VALUE wrap_long(CFNumberRef const num)      { WRAP_NUM(long,      kCFNumberLongType,     LONG2FIX) }
VALUE wrap_long_long(CFNumberRef const num) { WRAP_NUM(long long, kCFNumberLongLongType, LL2NUM)   }
VALUE wrap_float(CFNumberRef const num)     { WRAP_NUM(double,    kCFNumberDoubleType,   DBL2NUM)  }

#define UNWRAP_NUM(type, cookie, macro)         \
    const type base = macro(num);		\
    return CFNumberCreate(NULL, cookie, &base);

CFNumberRef unwrap_long(const VALUE num)      { UNWRAP_NUM(long,      kCFNumberLongType,     NUM2LONG) }
CFNumberRef unwrap_long_long(const VALUE num) { UNWRAP_NUM(long long, kCFNumberLongLongType, NUM2LL)   }
CFNumberRef unwrap_float(const VALUE num)     { UNWRAP_NUM(double,    kCFNumberDoubleType,   NUM2DBL)  }

VALUE
wrap_number(CFNumberRef const number)
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
unwrap_number(const VALUE number)
{
  switch (TYPE(number))
    {
    case T_FIXNUM:
      return unwrap_long(number);
    case T_FLOAT:
      return unwrap_float(number);
    default: {
        volatile VALUE num = number;
        rb_raise(rb_eRuntimeError,
                 "wrapping %s is not supported; log a bug?",
                 rb_string_value_cstr(&num));
        return kCFNumberNegativeInfinity; // unreachable
    }
    }
}

VALUE wrap_array_numbers(CFArrayRef const array) { WRAP_ARRAY(wrap_number) }



VALUE
wrap_url(CFURLRef const url)
{
  // @note CFURLGetString does not need to be CFReleased since it is a Get
  return rb_funcall(rb_mURI, sel_parse, 1, wrap_string(CFURLGetString(url)));
}

VALUE
wrap_nsurl(NSURL* const url)
{
  NSString* const str = [url absoluteString];
  const VALUE  rb_str = wrap_nsstring(str);
  [str release];
  return rb_funcall(rb_mURI, sel_parse, 1, rb_str);
}

CFURLRef
unwrap_url(const VALUE url)
{
  // TODO: should also force encoding to UTF-8 first?
    VALUE url_string = rb_funcall(url, sel_to_s, 0);
    CFStringRef const string =
        CFStringCreateWithCString(NULL,
                                  StringValuePtr(url_string),
                                  kCFStringEncodingUTF8);
    CFURLRef const url_ref = CFURLCreateWithString(NULL, string, NULL);
    CFRelease(string);
    return url_ref;
}

NSURL*
unwrap_nsurl(const VALUE url)
{
  return (NSURL*)unwrap_url(url);
}

VALUE wrap_array_urls(CFArrayRef const array) { WRAP_ARRAY(wrap_url) }


VALUE
wrap_date(CFDateRef const date)
{
    const NSTimeInterval time = [(NSDate*)date timeIntervalSince1970];
    return rb_time_new((time_t)time, 0);
}

VALUE
wrap_nsdate(NSDate* const date)
{
  return wrap_date((CFDateRef)date);
}

CFDateRef
unwrap_date(const VALUE date)
{
    const struct timeval t = rb_time_timeval(date);
    NSDate* const ns_date = [NSDate dateWithTimeIntervalSince1970:t.tv_sec];
    return (CFDateRef)ns_date;
}

VALUE wrap_array_dates(CFArrayRef const array) { WRAP_ARRAY(wrap_date) }


VALUE
wrap_boolean(CFBooleanRef const bool_val)
{
  return (CFBooleanGetValue(bool_val) ? Qtrue : Qfalse);
}

CFBooleanRef
unwrap_boolean(const VALUE bool_val)
{
  return (bool_val == Qtrue ? kCFBooleanTrue : kCFBooleanFalse);
}

VALUE wrap_array_booleans(CFArrayRef const array) { WRAP_ARRAY(wrap_boolean) }


VALUE
wrap_array(CFArrayRef const array)
{
    const CFIndex length = CFArrayGetCount(array);
    if (length) {
        const CFTypeRef obj = CFArrayGetValueAtIndex(array, 0);
        const CFTypeID   di = CFGetTypeID(obj);
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
wrap_dictionary(NSDictionary* const dict)
{
    const VALUE hash = rb_hash_new();

    [dict enumerateKeysAndObjectsUsingBlock:
     ^(const id key, const id obj, BOOL* const stop) {
            rb_hash_aset(hash, to_ruby(key), to_ruby(obj));
        }];

  return hash;
}

VALUE wrap_array_dictionaries(CFArrayRef const array) { WRAP_ARRAY(wrap_dictionary); }


VALUE
to_ruby(CFTypeRef const obj)
{
    const CFTypeID di = CFGetTypeID(obj);
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
to_ax(const VALUE obj)
{
    switch (TYPE(obj)) {
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

    const VALUE type = CLASS_OF(obj);
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
wrap_data(CFDataRef const data)
{
  return wrap_nsdata((NSData*)data);
}

VALUE
wrap_nsdata(NSData* const obj)
{
  WRAP_OBJC(rb_cData, objc_finalizer);
}

VALUE wrap_array_data(CFArrayRef const array) { WRAP_ARRAY(wrap_data); }
VALUE
wrap_array_nsdata(NSArray* const ary)
{
  CFArrayRef const array = (CFArrayRef)ary;
  WRAP_ARRAY(wrap_data);
}

CFDataRef
unwrap_data(const VALUE data)
{
  return (CFDataRef)unwrap_nsdata(data);
}

NSData*
unwrap_nsdata(const VALUE obj)
{
  UNWRAP_OBJC(NSData);
}


static
VALUE
rb_data_data(const VALUE self)
{
  return wrap_nsdata([NSData data]);
}

static
VALUE
rb_data_with_contents_of_url(const VALUE self, const VALUE url)
{
    NSData* const data = [NSData dataWithContentsOfURL:unwrap_nsurl(url)];
    if (data)
        return wrap_nsdata(data);
    return Qnil;
}

static
VALUE
rb_data_length(const VALUE self)
{
  return ULONG2NUM([unwrap_nsdata(self) length]);
}

static
VALUE
rb_data_equality(const VALUE self, const VALUE other)
{
  OBJC_EQUALITY(rb_cData, unwrap_nsdata);
}

static
VALUE
rb_data_write_to_file(const int argc, VALUE* const argv, const VALUE self)
{
    if (argc < 2)
        rb_raise(rb_eArgError,
                 "wrong number of arguments, got %d, expected 2",
                 argc);

    NSString* const path = unwrap_nsstring(argv[0]);
    const BOOL    result = [unwrap_nsdata(self) writeToFile:path
                                                 atomically:(argv[1] == Qtrue)];

    [path release];

    return (result ? Qtrue : Qfalse);
}


static
VALUE
rb_data_to_str(const VALUE self)
{
    NSData* const      data = unwrap_nsdata(self);
    const void* const bytes = [data bytes];
    const NSUInteger length = [data length];
    return rb_enc_str_new(bytes, length, rb_utf8_encoding());
}

static
VALUE
rb_str_to_data(const VALUE self)
{
    VALUE self_string = self;
    NSData* const data = [NSData dataWithBytes:(void*)StringValuePtr(self_string)
                                        length:RSTRING_LEN(self)];
    if (data)
        return wrap_nsdata(data);
    return Qnil; // I don't think this is possible except in case of ENOMEM
}


static VALUE
rb_spin(const int argc, VALUE* const argv, const VALUE self)
{
    if (argc == 0)
        spin(0);
    else
        spin(NUM2DBL(argv[0]));

    return self;
}


void
Init_bridge()
{
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
    rb_define_method(rb_cAttributedString, "initWithString", rb_astring_init_with_string, -1);
    /* rb_define_method(rb_cAttributedString, "initWithAttributedString", rb_astring_init_with_astring, 2); */
    rb_define_method(rb_cAttributedString, "string", rb_astring_string, 0);
    rb_define_method(rb_cAttributedString, "length", rb_astring_length, 0);
    /* rb_define_method(rb_cAttributedString, "attributesAtIndex", rb_astring_attrs_at_index, -1); */
    rb_define_method(rb_cAttributedString, "isEqualToAttributedString", rb_astring_equality, 1);
    /* rb_define_method(rb_cAttributedString, "attributedSubstringFromRange", rb_astring_astring_from_range, 1); */

    rb_define_alias(rb_cAttributedString, "to_s",   "string");
    rb_define_alias(rb_cAttributedString, "to_str", "string");
    rb_define_alias(rb_cAttributedString, "==",     "isEqualToAttributedString");


    /*
     * Document-class: NSData
     *
     * A 50% drop-in replacement for Cocoa's `NSData` class. Almost all
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

    rb_define_alias(rb_cData,  "==", "isEqualToData");


    // misc freedom patches
    rb_define_method(rb_cString, "to_data", rb_str_to_data, 0);
    rb_define_method(rb_cObject, "spin", rb_spin, -1); // semi-private method
}
