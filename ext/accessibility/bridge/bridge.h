#include "ruby.h"
#import <Cocoa/Cocoa.h>


// these functions are available on MacRuby as well as MRI
void spin(double seconds);

// initialize all the dynamic data (e.g. class pointers)
void Init_bridge();


#ifdef NOT_MACRUBY

extern VALUE rb_cData;
extern VALUE rb_cAttributedString;
extern VALUE rb_mAccessibility;
extern VALUE rb_cElement;
extern VALUE rb_cCGPoint;
extern VALUE rb_cCGSize;
extern VALUE rb_cCGRect;
extern VALUE rb_mURI; // URI module
extern VALUE rb_cURI; // URI::Generic class
extern VALUE rb_cScreen;

extern ID sel_x;
extern ID sel_y;
extern ID sel_width;
extern ID sel_height;
extern ID sel_origin;
extern ID sel_size;
extern ID sel_to_point;
extern ID sel_to_size;
extern ID sel_to_rect;
extern ID sel_to_s;
extern ID sel_parse;


#define WRAP_OBJC(klass, finalizer) do {				\
    return Data_Wrap_Struct(klass, NULL, finalizer, (void*)obj);	\
  } while (false);

#define UNWRAP_OBJC(klass) do {			\
    klass* unwrapped;				\
    Data_Get_Struct(obj, klass, unwrapped);	\
    return unwrapped;				\
  } while (false);

#define OBJC_EQUALITY(type, unwrapper) do {		\
    if (CLASS_OF(other) == type)			\
      if ([unwrapper(self) isEqual:unwrapper(other)])	\
	return Qtrue;					\
    return Qfalse;					\
  } while (false);

#define WRAP_ARRAY(wrapper) do {				\
    CFTypeRef  obj = NULL;				        \
    CFIndex length = CFArrayGetCount(array);			\
    VALUE      ary = rb_ary_new2(length);			\
    								\
    for (CFIndex idx = 0; idx < length; idx++) {		\
      obj = CFArrayGetValueAtIndex(array, idx);			\
      CFRetain(obj);						\
      rb_ary_store(ary, idx, wrapper(obj));			\
    }								\
    								\
    return ary;							\
  } while (false);


void cf_finalizer(void* obj);
void objc_finalizer(void* obj);

VALUE wrap_unknown(CFTypeRef obj);
CFTypeRef unwrap_unknown(VALUE obj);

VALUE wrap_point(CGPoint point);
CGPoint unwrap_point(VALUE point);

VALUE wrap_size(CGSize size);
CGSize unwrap_size(VALUE size);

VALUE wrap_rect(CGRect rect);
VALUE coerce_to_rect(VALUE obj);
CGRect unwrap_rect(VALUE rect);

VALUE convert_cf_range(CFRange range);
CFRange convert_rb_range(VALUE range);

VALUE wrap_value_point(AXValueRef value);
VALUE wrap_value_size(AXValueRef value);
VALUE wrap_value_rect(AXValueRef value);
VALUE wrap_value_range(AXValueRef value);
VALUE wrap_value_error(AXValueRef value);
VALUE wrap_value(AXValueRef value);
VALUE wrap_array_values(CFArrayRef array);

AXValueRef unwrap_value_point(VALUE val);
AXValueRef unwrap_value_size(VALUE val);
AXValueRef unwrap_value_rect(VALUE val);
AXValueRef unwrap_value_range(VALUE val);
AXValueRef unwrap_value(VALUE value);

VALUE wrap_ref(AXUIElementRef ref);
VALUE wrap_array_refs(CFArrayRef array);
AXUIElementRef unwrap_ref(VALUE obj);

VALUE wrap_string(CFStringRef string);
VALUE wrap_nsstring(NSString* string);
VALUE wrap_array_strings(CFArrayRef array);
VALUE wrap_array_nsstrings(NSArray* ary);
CFStringRef unwrap_string(VALUE string);
NSString*   unwrap_nsstring(VALUE string);

VALUE wrap_long(CFNumberRef num);
VALUE wrap_long_long(CFNumberRef num);
VALUE wrap_float(CFNumberRef num);
// Generic CFNumber wrapper, use it if you do not
// know the primitive type of number
VALUE wrap_number(CFNumberRef number);
VALUE wrap_array_numbers(CFArrayRef array);

CFNumberRef unwrap_long(VALUE num);
CFNumberRef unwrap_long_long(VALUE num);
CFNumberRef unwrap_float(VALUE num);
CFNumberRef unwrap_number(VALUE number);

VALUE wrap_url(CFURLRef url);
VALUE wrap_nsurl(NSURL* url);
CFURLRef unwrap_url(VALUE url);
NSURL* unwrap_nsurl(VALUE url);
VALUE wrap_array_urls(CFArrayRef array);

VALUE wrap_date(CFDateRef date);
VALUE wrap_nsdate(NSDate* date);
VALUE wrap_array_dates(CFArrayRef array);
CFDateRef unwrap_date(VALUE date);

VALUE wrap_boolean(CFBooleanRef bool_val);
VALUE wrap_array_booleans(CFArrayRef array);
CFBooleanRef unwrap_boolean(VALUE bool_val);

VALUE wrap_data(CFDataRef data);
VALUE wrap_nsdata(NSData* data);
CFDataRef unwrap_data(VALUE data);
NSData* unwrap_nsdata(VALUE data);

// this function assumes that arrays are homogeneous;
// which is usually the case coming from the CF world
VALUE wrap_array(CFArrayRef array);

VALUE wrap_dictionary(NSDictionary* dict);
VALUE wrap_array_dictionaries(CFArrayRef array);

VALUE to_ruby(CFTypeRef obj);
CFTypeRef to_ax(VALUE obj);

VALUE wrap_screen(NSScreen* screen);
VALUE wrap_array_screens(CFArrayRef array);
NSScreen* unwrap_screen(VALUE screen);

VALUE wrap_attributed_string(CFAttributedStringRef string);
VALUE wrap_nsattributed_string(NSAttributedString* string);
VALUE wrap_array_attributed_strings(CFArrayRef array);
VALUE wrap_array_nsattributed_strings(NSArray* ary);
CFAttributedStringRef unwrap_attributed_string(VALUE string);
NSAttributedString* unwrap_nsattributed_string(VALUE string);

VALUE wrap_data(CFDataRef data);
VALUE wrap_nsdata(NSData* data);
VALUE wrap_array_data(CFArrayRef array);
VALUE wrap_array_nsdata(NSArray* array);
CFDataRef unwrap_data(VALUE data);
NSData* unwrap_nsdata(VALUE data);

#endif
