#include "ruby.h"
#import <Cocoa/Cocoa.h>
#include "../bridge/bridge.h"
#include "../extras/extras.h"


#ifdef NOT_MACRUBY

static VALUE rb_cHighlighter;
static VALUE rb_cColor;

static ID ivar_color;

static VALUE color_key;
static VALUE colour_key;
static VALUE timeout_key;


static
VALUE
wrap_window(NSWindow* window)
{
  return Data_Wrap_Struct(rb_cHighlighter, NULL, objc_finalizer, (void*)window);
}

static
NSWindow*
unwrap_window(VALUE window)
{
  NSWindow* nswindow;
  Data_Get_Struct(window, NSWindow, nswindow);
  return nswindow;
}

static
VALUE
wrap_color(NSColor* color)
{
  return Data_Wrap_Struct(rb_cColor, NULL, objc_finalizer, (void*)color);
}

static
NSColor*
unwrap_color(VALUE color)
{
  NSColor* nscolor;
  Data_Get_Struct(color, NSColor, nscolor);
  return nscolor;
}


static
CGRect
flip(CGRect rect)
{
  double screen_height = NSMaxY([[NSScreen mainScreen] frame]);
  rect.origin.y        = screen_height - NSMaxY(rect);
  return rect;
}

static
VALUE
rb_highlighter_new(int argc, VALUE* argv, VALUE self)
{
  if (!argc)
    rb_raise(rb_eArgError, "wrong number of arguments (0 for 1+)");

  CGRect bounds = unwrap_rect(coerce_to_rect(argv[0]));
  bounds = flip(bounds); // we assume the rect is in the other co-ordinate system

  NSWindow* window =
    [[NSWindow alloc] initWithContentRect:bounds
                                styleMask:NSBorderlessWindowMask
		                  backing:NSBackingStoreBuffered
                                    defer:true];

  NSColor* color = [NSColor magentaColor];

  if (argc > 1) {
    VALUE rb_color = rb_hash_lookup(argv[1], color_key);
    if (rb_color == Qnil)
      rb_color = rb_hash_lookup(argv[1], colour_key);
    if (rb_color != Qnil)
      color = unwrap_color(rb_color);
  }

  [window setOpaque:false];
  [window setAlphaValue:0.20];
  [window setLevel:NSStatusWindowLevel];
  [window setBackgroundColor:color];
  [window setIgnoresMouseEvents:true];
  [window setFrame:bounds display:false];
  [window makeKeyAndOrderFront:NSApp];
  [window setReleasedWhenClosed:false];

  if (argc > 1) {
    VALUE rb_timeout = rb_hash_lookup(argv[1], timeout_key);
    if (rb_timeout != Qnil) {
      dispatch_time_t timeout = dispatch_time(
					      DISPATCH_TIME_NOW,
					      NUM2LL(rb_timeout) * NSEC_PER_SEC
					      );
      dispatch_after(
		     timeout,
		     dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
		     ^(void) { [window close]; }
		     );
    }
  }

  VALUE highlighter = wrap_window(window);
  rb_ivar_set(highlighter, ivar_color, wrap_color(color));
  return highlighter;
}

static
VALUE
rb_highlighter_stop(VALUE self)
{
  [unwrap_window(self) close];
  return self;
}

static
VALUE
rb_highlighter_color(VALUE self)
{
  return wrap_color([unwrap_window(self) backgroundColor]);
}

static
VALUE
rb_highlighter_frame(VALUE self)
{
  return wrap_rect([unwrap_window(self) frame]);
}

static
VALUE
rb_highlighter_is_visible(VALUE self)
{
  return ([unwrap_window(self) isVisible] ? Qtrue : Qfalse);
}


/*
 * @return [NSColor]
 */
static VALUE rb_color_black(VALUE self)      { return wrap_color([NSColor blackColor]);     }
/*
 * @return [NSColor]
 */
static VALUE rb_color_blue(VALUE self)       { return wrap_color([NSColor blueColor]);      }
/*
 * @return [NSColor]
 */
static VALUE rb_color_brown(VALUE self)      { return wrap_color([NSColor brownColor]);     }
/*
 * @return [NSColor]
 */
static VALUE rb_color_clear(VALUE self)      { return wrap_color([NSColor clearColor]);     }
/*
 * @return [NSColor]
 */
static VALUE rb_color_cyan(VALUE self)       { return wrap_color([NSColor cyanColor]);      }
/*
 * @return [NSColor]
 */
static VALUE rb_color_dark_gray(VALUE self)  { return wrap_color([NSColor darkGrayColor]);  }
/*
 * @return [NSColor]
 */
static VALUE rb_color_gray(VALUE self)       { return wrap_color([NSColor grayColor]);      }
/*
 * @return [NSColor]
 */
static VALUE rb_color_green(VALUE self)      { return wrap_color([NSColor greenColor]);     }
/*
 * @return [NSColor]
 */
static VALUE rb_color_light_gray(VALUE self) { return wrap_color([NSColor lightGrayColor]); }
/*
 * @return [NSColor]
 */
static VALUE rb_color_magenta(VALUE self)    { return wrap_color([NSColor magentaColor]);   }
/*
 * @return [NSColor]
 */
static VALUE rb_color_orange(VALUE self)     { return wrap_color([NSColor orangeColor]);    }
/*
 * @return [NSColor]
 */
static VALUE rb_color_purple(VALUE self)     { return wrap_color([NSColor purpleColor]);    }
/*
 * @return [NSColor]
 */
static VALUE rb_color_red(VALUE self)        { return wrap_color([NSColor redColor]);       }
/*
 * @return [NSColor]
 */
static VALUE rb_color_white(VALUE self)      { return wrap_color([NSColor whiteColor]);     }
/*
 * @return [NSColor]
 */
static VALUE rb_color_yellow(VALUE self)     { return wrap_color([NSColor yellowColor]);    }

/* static */
/* VALUE */
/* rb_color_rgb(VALUE self, VALUE red_val, VALUE other_vals) */
/* { */
/*   return Qnil; */
/* } */

/*
 * @return [Boolean]
 */
static
VALUE
rb_color_equality(VALUE self, VALUE other)
{
  if (CLASS_OF(other) == rb_cColor)
    if ([unwrap_color(self) isEqual:unwrap_color(other)])
      return Qtrue;

  return Qfalse;
}

#endif


void
Init_highlighter()
{
  Init_bridge();
  Init_extras();

#ifdef NOT_MACRUBY

  // force initialization or NSWindow won't work
  [NSApplication sharedApplication];

  // TODO: can we replace this bs with dispatch_once?
  rb_mAccessibility = rb_define_module("Accessibility");
  rb_cCGPoint       = rb_const_get(rb_cObject, rb_intern("CGPoint"));
  rb_cCGSize        = rb_const_get(rb_cObject, rb_intern("CGSize"));
  rb_cCGRect        = rb_const_get(rb_cObject, rb_intern("CGRect"));


  rb_cHighlighter = rb_define_class_under(rb_mAccessibility, "Highlighter", rb_cObject);

  rb_define_singleton_method(rb_cHighlighter, "new",  rb_highlighter_new,  -1);

  rb_define_method(rb_cHighlighter, "stop",     rb_highlighter_stop,       0);
  rb_define_method(rb_cHighlighter, "color",    rb_highlighter_color,      0);
  rb_define_method(rb_cHighlighter, "frame",    rb_highlighter_frame,      0);
  rb_define_method(rb_cHighlighter, "visible?", rb_highlighter_is_visible, 0);

  rb_define_alias(rb_cHighlighter, "colour", "color");

  ivar_color  = rb_intern("color");

  color_key   = ID2SYM(rb_intern("color"));
  colour_key  = ID2SYM(rb_intern("colour")); // fuck yeah, Canada
  timeout_key = ID2SYM(rb_intern("timeout"));


  /*
   * Document-class: NSColor
   *
   * A subset of Cocoa's `NSColor` class.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/ApplicationKit/Classes/NSColor_Class/Reference/Reference.html)
   * for documentation on the methods available in this class.
   */
  rb_cColor = rb_define_class("NSColor", rb_cObject);

  rb_define_singleton_method(rb_cColor, "blackColor",       rb_color_black,      0);
  rb_define_singleton_method(rb_cColor, "blueColor",        rb_color_blue,       0);
  rb_define_singleton_method(rb_cColor, "brownColor",       rb_color_brown,      0);
  rb_define_singleton_method(rb_cColor, "clearColor",       rb_color_clear,      0);
  rb_define_singleton_method(rb_cColor, "cyanColor",        rb_color_cyan,       0);
  rb_define_singleton_method(rb_cColor, "darkGrayColor",    rb_color_dark_gray,  0);
  rb_define_singleton_method(rb_cColor, "grayColor",        rb_color_gray,       0);
  rb_define_singleton_method(rb_cColor, "greenColor",       rb_color_green,      0);
  rb_define_singleton_method(rb_cColor, "lightGrayColor",   rb_color_light_gray, 0);
  rb_define_singleton_method(rb_cColor, "magentaColor",     rb_color_magenta,    0);
  rb_define_singleton_method(rb_cColor, "orangeColor",      rb_color_orange,     0);
  rb_define_singleton_method(rb_cColor, "purpleColor",      rb_color_purple,     0);
  rb_define_singleton_method(rb_cColor, "redColor",         rb_color_red,        0);
  rb_define_singleton_method(rb_cColor, "whiteColor",       rb_color_white,      0);
  rb_define_singleton_method(rb_cColor, "yellowColor",      rb_color_yellow,     0);
  //rb_define_singleton_method(rb_cColor, "colorWithSRGBRed", rb_color_rgb,        2);

  rb_define_method(rb_cColor, "==", rb_color_equality, 1);

#endif
}
