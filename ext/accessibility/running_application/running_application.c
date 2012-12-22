#include "ruby.h"
#import <Cocoa/Cocoa.h>
#include "../bridge/bridge.c"

static VALUE rb_cRunningApp;
static VALUE rb_cWorkspace;

static ID key_opts;
static ID key_event_params;
static ID key_launch_id;

static
void
objc_finalizer(void* obj)
{
  [(id)obj release];
}

static
VALUE
wrap_app(NSRunningApplication* app)
{
  return Data_Wrap_Struct(rb_cRunningApp, NULL, objc_finalizer, (void*)app);
}

static
NSRunningApplication*
unwrap_app(VALUE app)
{
  NSRunningApplication* running_app;
  Data_Get_Struct(app, NSRunningApplication, running_app);
  return running_app;
}

static VALUE wrap_array_apps(CFArrayRef array) { WRAP_ARRAY(wrap_app); }


static
VALUE
rb_running_app_with_pid(VALUE self, VALUE pid)
{
  return wrap_app([NSRunningApplication
		   runningApplicationWithProcessIdentifier:NUM2PIDT(pid)]);
}

static
VALUE
rb_running_app_with_bundle_id(VALUE self, VALUE bundle_id)
{
  return wrap_array_apps((CFArrayRef)[NSRunningApplication
		   runningApplicationsWithBundleIdentifier:unwrap_nsstring(bundle_id)]);
}

static
VALUE
rb_running_app_current_app(VALUE self)
{
  return wrap_app([NSRunningApplication currentApplication]);
}

static
VALUE
rb_running_app_drop_nuke(VALUE self)
{
  [NSRunningApplication terminateAutomaticallyTerminableApplications];
  return rb_cRunningApp; // return this to be MacRuby compatible
}


static
VALUE
rb_running_app_is_active(VALUE self)
{
  return unwrap_app(self).isActive ? Qtrue : Qfalse;
}

static
VALUE
rb_running_app_activate(VALUE self, VALUE options)
{
  return ([unwrap_app(self) activateWithOptions:FIX2INT(options)] ? Qtrue : Qfalse);
}

static
VALUE
rb_running_app_activation_policy(VALUE self)
{
  return INT2FIX(unwrap_app(self).activationPolicy);
}

static
VALUE
rb_running_app_hide(VALUE self)
{
  return ([unwrap_app(self) hide] ? Qtrue : Qfalse);
}

static
VALUE
rb_running_app_is_hidden(VALUE self)
{
  return (unwrap_app(self).isHidden ? Qtrue : Qfalse);
}

static
VALUE
rb_running_app_localized_name(VALUE self)
{
  return wrap_string((CFStringRef)[unwrap_app(self) localizedName]);
}

static
VALUE
rb_running_app_bundle_id(VALUE self)
{
  return wrap_string((CFStringRef)[unwrap_app(self) bundleIdentifier]);
}

static
VALUE
rb_running_app_bundle_url(VALUE self)
{
  return wrap_url((CFURLRef)[unwrap_app(self) bundleURL]);
}

static
VALUE
rb_running_app_executable_arch(VALUE self)
{
  return INT2FIX(unwrap_app(self).executableArchitecture);
}

static
VALUE
rb_running_app_executable_url(VALUE self)
{
  return wrap_nsurl(unwrap_app(self).executableURL);
}

static
VALUE
rb_running_app_launch_date(VALUE self)
{
  return wrap_nsdate(unwrap_app(self).launchDate);
}

static
VALUE
rb_running_app_is_launched(VALUE self)
{
  return (unwrap_app(self).isFinishedLaunching ? Qtrue : Qfalse);
}

static
VALUE
rb_running_app_pid(VALUE self)
{
  return PIDT2NUM(unwrap_app(self).processIdentifier);
}

static
VALUE
rb_running_app_owns_menu_bar(VALUE self)
{
  return (unwrap_app(self).ownsMenuBar ? Qtrue : Qfalse);
}

static
VALUE
rb_running_app_force_terminate(VALUE self)
{
  return ([unwrap_app(self) forceTerminate] ? Qtrue : Qfalse);
}

static
VALUE
rb_running_app_terminate(VALUE self)
{
  return ([unwrap_app(self) terminate] ? Qtrue : Qfalse);
}

static
VALUE
rb_running_app_is_terminated(VALUE self)
{
  return (unwrap_app(self).isTerminated ? Qtrue : Qfalse);
}


static
VALUE
rb_workspace_shared(VALUE self)
{
  return self;
}

static
VALUE
rb_workspace_frontmost_app(VALUE self)
{
  return wrap_app([[NSWorkspace sharedWorkspace] frontmostApplication]);
}

static
VALUE
rb_workspace_menu_bar_owner(VALUE self)
{
  return wrap_app([[NSWorkspace sharedWorkspace] menuBarOwningApplication]);
}

static
VALUE
rb_workspace_find(VALUE self, VALUE query)
{
  BOOL result =
    [[NSWorkspace sharedWorkspace] showSearchResultsForQueryString:unwrap_nsstring(query)];
  return (result ? Qtrue : Qfalse);
}


void
Init_running_application()
{
  rb_cRunningApp = rb_define_class("NSRunningApplication", rb_cObject);

  rb_define_singleton_method(rb_cRunningApp, "runningApplicationWithProcessIdentifier",      rb_running_app_with_pid,       1);
  rb_define_singleton_method(rb_cRunningApp, "runningApplicationsWithBundleIdentifier",      rb_running_app_with_bundle_id, 1);
  rb_define_singleton_method(rb_cRunningApp, "currentApplication",                           rb_running_app_current_app,    0);
  rb_define_singleton_method(rb_cRunningApp, "terminateAutomaticallyTerminableApplications", rb_running_app_drop_nuke, 0);

  rb_define_method(rb_cRunningApp, "active?",                rb_running_app_is_active,         0);
  rb_define_method(rb_cRunningApp, "activateWithOptions",    rb_running_app_activate,          1);
  rb_define_method(rb_cRunningApp, "activationPolicy",       rb_running_app_activation_policy, 0);
  rb_define_method(rb_cRunningApp, "hide",                   rb_running_app_hide,              0);
  rb_define_method(rb_cRunningApp, "unhide",                 rb_running_app_hide,              0);
  rb_define_method(rb_cRunningApp, "hidden?",                rb_running_app_is_hidden,         0);
  rb_define_method(rb_cRunningApp, "localizedName",          rb_running_app_localized_name,    0);
  //rb_define_method(rb_cRunningApp, "icon",                   rb_running_app_icon,              0);
  rb_define_method(rb_cRunningApp, "bundleIdentifier",       rb_running_app_bundle_id,         0);
  rb_define_method(rb_cRunningApp, "bundleURL",              rb_running_app_bundle_url,        0);
  rb_define_method(rb_cRunningApp, "executableArchitecture", rb_running_app_executable_arch,   0);
  rb_define_method(rb_cRunningApp, "executableURL",          rb_running_app_executable_url,    0);
  rb_define_method(rb_cRunningApp, "launchDate",             rb_running_app_launch_date,       0);
  rb_define_method(rb_cRunningApp, "finishedLaunching?",     rb_running_app_is_launched,       0);
  rb_define_method(rb_cRunningApp, "processIdentifier",      rb_running_app_pid,               0);
  rb_define_method(rb_cRunningApp, "ownsMenuBar",            rb_running_app_owns_menu_bar,     0);
  rb_define_alias( rb_cRunningApp, "ownsMenuBar?", "ownsMenuBar");
  rb_define_method(rb_cRunningApp, "forceTerminate",         rb_running_app_force_terminate,   0);
  rb_define_method(rb_cRunningApp, "terminate",              rb_running_app_terminate,         0);
  rb_define_method(rb_cRunningApp, "terminated?",            rb_running_app_is_terminated,     0);


  rb_cWorkspace = rb_define_class("NSWorkspace", rb_cObject);

  rb_define_singleton_method(rb_cWorkspace, "sharedWorkspace",       rb_workspace_shared,         0);

  //  rb_define_method(rb_cWorkspace, "launchAppWithBundleIdentifier",   rb_workspace_launch,         2);
  rb_define_singleton_method(rb_cWorkspace, "frontmostApplication",            rb_workspace_frontmost_app,  0);
  rb_define_singleton_method(rb_cWorkspace, "menuBarOwningApplication",        rb_workspace_menu_bar_owner, 0);
  rb_define_singleton_method(rb_cWorkspace, "showSearchResultsForQueryString", rb_workspace_find,           1);

  key_opts         = rb_intern("options");
  key_event_params = rb_intern("additionalEventParamDescriptor");
  key_launch_id    = rb_intern("launchIdentifier");

  // Technically these are global constants, but in MacRuby you can still access them from any
  // namespace. So, we will try by first adding them to the NSRunningApplication namespae only
#define RUNNING_APP_CONST(str, val) rb_define_const(rb_cRunningApp, str, INT2FIX(val));
  RUNNING_APP_CONST("NSApplicationActivateAllWindows",        NSApplicationActivateAllWindows);
  RUNNING_APP_CONST("NSApplicationActivateIgnoringOtherApps", NSApplicationActivateIgnoringOtherApps);
  RUNNING_APP_CONST("NSBundleExecutableArchitectureI386",     NSBundleExecutableArchitectureI386);
  RUNNING_APP_CONST("NSBundleExecutableArchitecturePPC",      NSBundleExecutableArchitecturePPC);
  RUNNING_APP_CONST("NSBundleExecutableArchitectureX86_64",   NSBundleExecutableArchitectureX86_64);
  RUNNING_APP_CONST("NSBundleExecutableArchitecturePPC64",    NSBundleExecutableArchitecturePPC64);
}
