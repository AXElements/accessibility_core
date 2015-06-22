#include "ruby.h"
#import <Cocoa/Cocoa.h>
#include "../bridge/bridge.h"

static VALUE rb_mSS;

static
VALUE
rb_ss_take_shot(__unused const VALUE self, const VALUE rbrect, const VALUE path)
{
    NSString* const ns_path =
	[[NSString alloc] initWithBytesNoCopy:RSTRING_PTR(path)
	                               length:RSTRING_LEN(path)
	                             encoding:NSUTF8StringEncoding
	                         freeWhenDone:NO];

    CGRect rect = unwrap_rect(rbrect);
    if (rect.size.width < 0 || rect.size.height < 0)
	rect = CGRectInfinite;

    CGImageRef const image =
	CGWindowListCreateImage(rect,
				kCGWindowListOptionOnScreenOnly,
				kCGNullWindowID,
				kCGWindowImageDefault);

    NSBitmapImageRep* const rep =
	[[NSBitmapImageRep alloc] initWithCGImage:image];

    NSData* const data = [rep representationUsingType:NSPNGFileType
			                   properties:nil];

    const VALUE result =
	[data writeToFile:ns_path atomically:NO] ? Qtrue : Qfalse;


    [ns_path release];
    if (image)
	CFRelease(image);
    [rep release];
    [data release];

    return result;
}

void Init_screen_shooter(void);

void
Init_screen_shooter()
{
    Init_bridge();

    /*
     * Document-module: ScreenShooter
     *
     * A module that adds a simple API for taking screen shots. It only
     * uses default options at the moment, so screen shots may not come
     * out as fancy as they could...this could be fixed in the future...
     */
    rb_mSS = rb_define_module("ScreenShooter");
    rb_extend_object(rb_mSS, rb_mSS);
    rb_define_method(rb_mSS, "screenshot", rb_ss_take_shot, 2);
}
