#include "ruby.h"
#import <Cocoa/Cocoa.h>
#include "../bridge/bridge.c"

#ifdef NOT_MACRUBY

static VALUE rb_cRunningApp;
static VALUE rb_cWorkspace;
static VALUE rb_cProcInfo;
static VALUE rb_cHost;

static VALUE key_opts;
//static VALUE key_event_params;
//static VALUE key_launch_id;

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

/*
 * @todo One thing we want to look into is using Launch Services instead of
 *       NSWorkspace for app launching. The reason is that we will avoid the
 *       problem launching new apps that have not been registered yet. But the
 *       big thing is that we can get the PSN for the application, which will
 *       allow for more directed input using CGEvents and it also gives us a
 *       data structure to wait on that indicates when the app has properly
 *       started up.
 *
 * The `additonalEventParamDescriptor` option is not supported by the bridge rigt now
 */
static
VALUE
rb_workspace_launch(VALUE self, VALUE bundle_id, VALUE opts)
{
  NSString*             identifier = unwrap_nsstring(bundle_id);
  NSWorkspaceLaunchOptions options = NUM2INT(rb_hash_lookup(opts, key_opts));

  BOOL result = [[NSWorkspace sharedWorkspace]
	        	 launchAppWithBundleIdentifier:identifier
        		                       options:options
        		additionalEventParamDescriptor:nil
                                      launchIdentifier:nil];

  [identifier release];
  return result ? Qtrue : Qfalse;
}


static
VALUE
rb_procinfo_self(VALUE self)
{
  return self;
}

static
VALUE
rb_procinfo_os_version(VALUE self)
{
  NSString* value = [[NSProcessInfo processInfo] operatingSystemVersionString];
  VALUE       obj = wrap_nsstring(value);
  [value release];
  return obj;
}

static
VALUE
rb_procinfo_sys_uptime(VALUE self)
{
  return DBL2NUM([[NSProcessInfo processInfo] systemUptime]);
}

static
VALUE
rb_procinfo_cpu_count(VALUE self)
{
  return INT2FIX([[NSProcessInfo processInfo] processorCount]);
}

static
VALUE
rb_procinfo_active_cpus(VALUE self)
{
  return INT2FIX([[NSProcessInfo processInfo] activeProcessorCount]);
}

static
VALUE
rb_procinfo_total_ram(VALUE self)
{
  return ULL2NUM([[NSProcessInfo processInfo] physicalMemory]);
}


static
VALUE
rb_host_self(VALUE self)
{
  return self;
}

static
VALUE
rb_host_names(VALUE self)
{
  NSArray* nsnames = [[NSHost currentHost] names];
  VALUE    rbnames = wrap_array_nsstrings(nsnames);
  [nsnames release];
  return rbnames;
}

static
VALUE
rb_host_addresses(VALUE self)
{
  NSArray* nsaddrs = [[NSHost currentHost] addresses];
  VALUE    rbaddrs = wrap_array_nsstrings(nsaddrs);
  [nsaddrs release];
  return rbaddrs;
}

static
VALUE
rb_host_localized_name(VALUE self)
{
  NSString* name = [[NSHost currentHost] localizedName];
  VALUE  rb_name = wrap_nsstring(name);
  [name release];
  return rb_name;
}

#endif


void
Init_running_application()
{
#ifdef NOT_MACRUBY
  /*
   * Document-class: NSRunningApplication
   *
   * A 99% drop-in replacement for Cocoa's `NSWorkspace` class on MRI and other
   * non-MacRuby rubies.
   *
   * See https://developer.apple.com/library/mac/#documentation/AppKit/Reference/NSRunningApplication_Class/Reference/Reference.html
   * for documentation on the methods in this class.
   */
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
  rb_define_alias(rb_cRunningApp, "ownsMenuBar?", "ownsMenuBar");
  rb_define_method(rb_cRunningApp, "forceTerminate",         rb_running_app_force_terminate,   0);
  rb_define_method(rb_cRunningApp, "terminate",              rb_running_app_terminate,         0);
  rb_define_method(rb_cRunningApp, "terminated?",            rb_running_app_is_terminated,     0);

  // Technically these are global constants, but in MacRuby you can still access them from any
  // namespace. So, we will try by first adding them to the NSRunningApplication namespae only
#define RUNNING_APP_CONST(str, val) rb_define_const(rb_cRunningApp, str, INT2FIX(val));
  RUNNING_APP_CONST("NSApplicationActivateAllWindows",        NSApplicationActivateAllWindows);
  RUNNING_APP_CONST("NSApplicationActivateIgnoringOtherApps", NSApplicationActivateIgnoringOtherApps);

  RUNNING_APP_CONST("NSBundleExecutableArchitectureI386",     NSBundleExecutableArchitectureI386);
  RUNNING_APP_CONST("NSBundleExecutableArchitecturePPC",      NSBundleExecutableArchitecturePPC);
  RUNNING_APP_CONST("NSBundleExecutableArchitectureX86_64",   NSBundleExecutableArchitectureX86_64);
  RUNNING_APP_CONST("NSBundleExecutableArchitecturePPC64",    NSBundleExecutableArchitecturePPC64);

  /*
   * Document-class: NSWorkspace
   *
   * A subset of Cocoa's `NSWorkspace` class.
   *
   * See https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/ApplicationKit/Classes/NSWorkspace_Class/Reference/Reference.html
   * for documentation on the methods available in this class.
   */
  rb_cWorkspace = rb_define_class("NSWorkspace", rb_cObject);

  rb_define_singleton_method(rb_cWorkspace, "sharedWorkspace",                 rb_workspace_shared,         0);
  rb_define_singleton_method(rb_cWorkspace, "frontmostApplication",            rb_workspace_frontmost_app,  0);
  rb_define_singleton_method(rb_cWorkspace, "menuBarOwningApplication",        rb_workspace_menu_bar_owner, 0);
  rb_define_singleton_method(rb_cWorkspace, "showSearchResultsForQueryString", rb_workspace_find,           1);
  rb_define_singleton_method(rb_cWorkspace, "launchAppWithBundleIdentifier",   rb_workspace_launch,         2);

  key_opts         = ID2SYM(rb_intern("options"));
  //  key_event_params = ID2SYM(rb_intern("additionalEventParamDescriptor"));
  //  key_launch_id    = ID2SYM(rb_intern("launchIdentifier"));

#define WORKSPACE_CONST(str, val) rb_define_const(rb_cWorkspace, str, INT2FIX(val));
  WORKSPACE_CONST("NSWorkspaceLaunchAndPrint",                 NSWorkspaceLaunchAndPrint);
  WORKSPACE_CONST("NSWorkspaceLaunchInhibitingBackgroundOnly", NSWorkspaceLaunchInhibitingBackgroundOnly);
  WORKSPACE_CONST("NSWorkspaceLaunchWithoutAddingToRecents",   NSWorkspaceLaunchWithoutAddingToRecents);
  WORKSPACE_CONST("NSWorkspaceLaunchWithoutActivation",        NSWorkspaceLaunchWithoutActivation);
  WORKSPACE_CONST("NSWorkspaceLaunchAsync",                    NSWorkspaceLaunchAsync);
  WORKSPACE_CONST("NSWorkspaceLaunchAllowingClassicStartup",   NSWorkspaceLaunchAllowingClassicStartup);
  WORKSPACE_CONST("NSWorkspaceLaunchPreferringClassic",        NSWorkspaceLaunchPreferringClassic);
  WORKSPACE_CONST("NSWorkspaceLaunchNewInstance",              NSWorkspaceLaunchNewInstance);
  WORKSPACE_CONST("NSWorkspaceLaunchAndHide",                  NSWorkspaceLaunchAndHide);
  WORKSPACE_CONST("NSWorkspaceLaunchAndHideOthers",            NSWorkspaceLaunchAndHideOthers);
  WORKSPACE_CONST("NSWorkspaceLaunchDefault",                  NSWorkspaceLaunchDefault);


  /*
   * Document-class: NSProcessInfo
   *
   * A subset of Cocoa's `NSProcessInfo` class. Methods that might be
   * useful to have been bridged.
   *
   * See https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSProcessInfo_Class/Reference/Reference.html
   * for documentation on the methods available in this class.
   */
  rb_cProcInfo = rb_define_class("NSProcessInfo", rb_cObject);

  rb_define_singleton_method(rb_cProcInfo, "processInfo",                  rb_procinfo_self,        0);
  rb_define_singleton_method(rb_cProcInfo, "operatingSystemVersionString", rb_procinfo_os_version,  0);
  rb_define_singleton_method(rb_cProcInfo, "systemUptime",                 rb_procinfo_sys_uptime,  0);
  rb_define_singleton_method(rb_cProcInfo, "processorCount",               rb_procinfo_cpu_count,   0);
  rb_define_singleton_method(rb_cProcInfo, "activeProcessorCount",         rb_procinfo_active_cpus, 0);
  rb_define_singleton_method(rb_cProcInfo, "physicalMemory",               rb_procinfo_total_ram,   0);


  /*
   * Document-class: NSHost
   *
   * A large subset of Cocoa's `NSHost` class. Methods that might be
   * useful to have been bridged.
   *
   * See https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSHost_Class/Reference/Reference.html
   * for documentation on the methods available in this class.
   */
  rb_cHost = rb_define_class("NSHost", rb_cObject);

  rb_define_singleton_method(rb_cHost, "currentHost",   rb_host_self,           0);
  rb_define_singleton_method(rb_cHost, "names",         rb_host_names,          0);
  rb_define_singleton_method(rb_cHost, "addresses",     rb_host_addresses,      0);
  rb_define_singleton_method(rb_cHost, "localizedName", rb_host_localized_name, 0);
#endif

}
