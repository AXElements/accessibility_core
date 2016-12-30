#pragma once

#include "ruby.h"
#import <Cocoa/Cocoa.h>

// these functions are available on MacRuby as well as MRI
void spin(const double seconds);

// initialize all the dynamic data (e.g. class pointers)
void Init_bridge();


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


#define WRAP_OBJC(klass, finalizer) \
    return Data_Wrap_Struct(klass, NULL, finalizer, (void*)obj);

#define UNWRAP_OBJC(klass)                      \
    klass* unwrapped;				\
    Data_Get_Struct(obj, klass, unwrapped);	\
    return unwrapped;

#define OBJC_EQUALITY(type, unwrapper)                  \
    if (CLASS_OF(other) == type)			\
      if ([unwrapper(self) isEqual:unwrapper(other)])	\
	return Qtrue;					\
    return Qfalse;

#define WRAP_ARRAY(wrapper)                                             \
    const CFIndex length = CFArrayGetCount(array);                      \
    const VALUE  new_ary = rb_ary_new2(length);                         \
                                                                        \
    for (CFIndex idx = 0; idx < length; idx++) {                        \
        CFTypeRef const obj = CFArrayGetValueAtIndex(array, idx);       \
        CFRetain(obj);                                                  \
        rb_ary_store(new_ary, idx, wrapper(obj));			\
    }                                                                   \
                                                                        \
    return new_ary;


static void __attribute__ ((unused))
cf_finalizer(void* obj)   { CFRelease((CFTypeRef)obj); }

static void __attribute__ ((unused))
objc_finalizer(void* obj) { [(id)obj release]; }

VALUE wrap_unknown(CFTypeRef const obj);
CFTypeRef unwrap_unknown(const VALUE obj);

VALUE wrap_point(const CGPoint point);
CGPoint unwrap_point(const VALUE point);

VALUE wrap_size(const CGSize size);
CGSize unwrap_size(const VALUE size);

VALUE wrap_rect(const CGRect rect);
VALUE coerce_to_rect(const VALUE obj);
CGRect unwrap_rect(const VALUE rect);

VALUE convert_cf_range(const CFRange range);
CFRange convert_rb_range(const VALUE range);

VALUE wrap_value_point(AXValueRef const value);
VALUE wrap_value_size(AXValueRef const value);
VALUE wrap_value_rect(AXValueRef const value);
VALUE wrap_value_range(AXValueRef const value);
VALUE wrap_value_error(AXValueRef const value);
VALUE wrap_value(AXValueRef const value);
VALUE wrap_array_values(CFArrayRef const array);

AXValueRef unwrap_value_point(const VALUE val);
AXValueRef unwrap_value_size(const VALUE val);
AXValueRef unwrap_value_rect(const VALUE val);
AXValueRef unwrap_value_range(const VALUE val);
AXValueRef unwrap_value(const VALUE value);

VALUE wrap_ref(AXUIElementRef const ref);
VALUE wrap_array_refs(CFArrayRef const array);
AXUIElementRef unwrap_ref(const VALUE obj);

VALUE wrap_string(CFStringRef const string);
VALUE wrap_nsstring(NSString* const string);
VALUE wrap_array_strings(CFArrayRef const array);
VALUE wrap_array_nsstrings(NSArray* const ary);
CFStringRef unwrap_string(const VALUE string);
NSString*   unwrap_nsstring(const VALUE string);

VALUE wrap_long(CFNumberRef const num);
VALUE wrap_long_long(CFNumberRef const num);
VALUE wrap_float(CFNumberRef const num);
// Generic CFNumber wrapper, use it if you do not
// know the primitive type of number
VALUE wrap_number(CFNumberRef const number);
VALUE wrap_array_numbers(CFArrayRef const array);

CFNumberRef unwrap_long(const VALUE num);
CFNumberRef unwrap_long_long(const VALUE num);
CFNumberRef unwrap_float(const VALUE num);
CFNumberRef unwrap_number(const VALUE number);

VALUE wrap_url(CFURLRef const url);
VALUE wrap_nsurl(NSURL* const url);
CFURLRef unwrap_url(const VALUE url);
NSURL* unwrap_nsurl(const VALUE url);
VALUE wrap_array_urls(CFArrayRef const array);

VALUE wrap_date(CFDateRef const date);
VALUE wrap_nsdate(NSDate* const date);
VALUE wrap_array_dates(CFArrayRef const array);
CFDateRef unwrap_date(const VALUE date);

VALUE wrap_boolean(CFBooleanRef const bool_val);
VALUE wrap_array_booleans(CFArrayRef const array);
CFBooleanRef unwrap_boolean(const VALUE bool_val);

VALUE wrap_data(CFDataRef const data);
VALUE wrap_nsdata(NSData* const data);
CFDataRef unwrap_data(const VALUE data);
NSData* unwrap_nsdata(const VALUE data);

// this function assumes that arrays are homogeneous;
// which is usually the case coming from the CF world
VALUE wrap_array(CFArrayRef const array);

VALUE wrap_dictionary(NSDictionary* const dict);
VALUE wrap_array_dictionaries(CFArrayRef const array);

VALUE to_ruby(CFTypeRef const obj);
CFTypeRef to_ax(const VALUE obj);

VALUE wrap_screen(NSScreen* const screen);
VALUE wrap_array_screens(CFArrayRef const array);
NSScreen* unwrap_screen(const VALUE screen);

VALUE wrap_attributed_string(CFAttributedStringRef const string);
VALUE wrap_nsattributed_string(NSAttributedString* const string);
VALUE wrap_array_attributed_strings(CFArrayRef const array);
VALUE wrap_array_nsattributed_strings(NSArray* const ary);
CFAttributedStringRef unwrap_attributed_string(const VALUE string);
NSAttributedString* unwrap_nsattributed_string(const VALUE string);

VALUE wrap_data(CFDataRef const data);
VALUE wrap_nsdata(NSData* const data);
VALUE wrap_array_data(CFArrayRef const array);
VALUE wrap_array_nsdata(NSArray* const array);
CFDataRef unwrap_data(const VALUE data);
NSData* unwrap_nsdata(const VALUE data);
