#include "ruby.h"
#import <Cocoa/Cocoa.h>


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


VALUE wrap_unknown(CFTypeRef obj);
CFTypeRef unwrap_unknown(VALUE obj);

VALUE wrap_point(CGPoint point);
CGPoint unwrap_point(VALUE point);

VALUE wrap_size(CGSize size);
CGSize unwrap_size(VALUE size);

VALUE wrap_rect(CGRect rect);
CGRect unwrap_rect(VALUE rect);

VALUE convert_cf_range(CFRange range);
CFRange convert_rb_range(VALUE range);


VALUE wrap_value_point(AXValueRef value);
VALUE wrap_value_size(AXValueRef value);
VALUE wrap_value_rect(AXValueRef value);
VALUE wrap_value_range(AXValueRef value);
VALUE wrap_value_error(AXValueRef value);

AXValueRef unwrap_value_point(VALUE val);
AXValueRef unwrap_value_size(VALUE val);
AXValueRef unwrap_value_rect(VALUE val);
AXValueRef unwrap_value_range(VALUE val);


VALUE wrap_value(AXValueRef value);
AXValueRef unwrap_value(VALUE value);
VALUE wrap_array_values(CFArrayRef array);


VALUE wrap_ref(AXUIElementRef ref);
AXUIElementRef unwrap_ref(VALUE obj);
VALUE wrap_array_refs(CFArrayRef array);


VALUE wrap_string(CFStringRef string);
CFStringRef unwrap_string(VALUE string);
VALUE wrap_array_strings(CFArrayRef array);


VALUE wrap_long(CFNumberRef num);
VALUE wrap_long_long(CFNumberRef num);
VALUE wrap_float(CFNumberRef num);

CFNumberRef unwrap_long(VALUE num);
CFNumberRef unwrap_long_long(VALUE num);
CFNumberRef unwrap_float(VALUE num);

VALUE wrap_number(CFNumberRef number);
CFNumberRef unwrap_number(VALUE number);
VALUE wrap_array_numbers(CFArrayRef array);


VALUE wrap_url(CFURLRef url);
CFURLRef unwrap_url(VALUE url);
VALUE wrap_array_urls(CFArrayRef array);


VALUE wrap_date(CFDateRef date);
CFDateRef unwrap_date(VALUE date);
VALUE wrap_array_dates(CFArrayRef array);


VALUE wrap_boolean(CFBooleanRef bool_val);
CFBooleanRef unwrap_boolean(VALUE bool_val);
VALUE wrap_array_booleans(CFArrayRef array);


VALUE wrap_array(CFArrayRef array);

VALUE to_ruby(CFTypeRef obj);
CFTypeRef to_ax(VALUE obj);
