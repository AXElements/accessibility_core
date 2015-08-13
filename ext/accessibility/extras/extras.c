#include "ruby.h"

#include "../bridge/bridge.h"

#import <IOKit/IOKitlib.h>
#import <IOKit/ps/IOPowerSources.h>
#import <IOKit/ps/IOPSKeys.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <IOKit/hidsystem/IOHIDShared.h>
#import <Cocoa/Cocoa.h>

static VALUE rb_mBattery;

static VALUE battery_not_installed;
static VALUE battery_charged;
static VALUE battery_charging;
static VALUE battery_discharging;

static io_connect_t screen_connection = MACH_PORT_NULL;

static VALUE rb_cRunningApp;
static VALUE rb_cWorkspace;
static VALUE rb_cProcInfo;
static VALUE rb_cHost;
static VALUE rb_cBundle;

static VALUE key_opts;
//static VALUE key_event_params;
//static VALUE key_launch_id;


static
VALUE
wrap_app(NSRunningApplication* const app)
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

static VALUE wrap_array_apps(NSArray* const ary)
{
  CFArrayRef const array = (CFArrayRef const)ary;
  WRAP_ARRAY(wrap_app);
}


static
VALUE
rb_running_app_with_pid(VALUE self, VALUE num_pid)
{
    const pid_t pid = NUM2PIDT(num_pid);
    NSRunningApplication* const app =
      [NSRunningApplication runningApplicationWithProcessIdentifier:pid];

    if (app) return wrap_app(app);
    return Qnil; // ruby behaviour would be to raise, but we want "drop-in" compat
}

static
VALUE
rb_running_app_with_bundle_id(VALUE self, VALUE bundle_id)
{
  return wrap_array_apps([NSRunningApplication
			  runningApplicationsWithBundleIdentifier:unwrap_nsstring(bundle_id)]);
}

static
VALUE
rb_running_app_current_app(VALUE self)
{
    NSRunningApplication* const app = [NSRunningApplication currentApplication];
    if (app) return wrap_app(app);
    return Qnil;
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
rb_running_app_unhide(VALUE self)
{
  return ([unwrap_app(self) unhide] ? Qtrue : Qfalse);
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
  NSString* string = [unwrap_app(self) localizedName];
  VALUE        str = Qnil;
  if (string)
    str = wrap_nsstring(string);
  [string release];
  return str;
}

static
VALUE
rb_running_app_bundle_id(VALUE self)
{
  NSString* name = [unwrap_app(self) bundleIdentifier];
  if (name)
    return wrap_nsstring(name);
  return Qnil;
}

static
VALUE
rb_running_app_bundle_url(VALUE self)
{
  NSURL* url  = [unwrap_app(self) bundleURL];
  VALUE rburl = Qnil;
  if (url)
    rburl = wrap_nsurl(url);
  [url release];
  return rburl;
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
rb_running_app_equality(VALUE self, VALUE other)
{
  if (CLASS_OF(other) == rb_cRunningApp)
    if ([unwrap_app(self) isEqual:unwrap_app(other)])
      return Qtrue;
  return Qfalse;
}


static
VALUE
rb_workspace_shared(VALUE self)
{
  return self;
}


static
VALUE
rb_workspace_running_apps(VALUE self)
{
  NSArray* apps = [[NSWorkspace sharedWorkspace] runningApplications];
  VALUE rb_apps = wrap_array_apps(apps);
  [apps enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
      [obj retain];
    }];
  [apps release];
  return rb_apps;
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
  return self; // hack
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


VALUE wrap_bundle(NSBundle* obj) { WRAP_OBJC(rb_cBundle, objc_finalizer); }
NSBundle* unwrap_bundle(VALUE obj) { UNWRAP_OBJC(NSBundle); }

static
VALUE
rb_bundle_with_url(VALUE self, VALUE url)
{
  NSURL*     nsurl = unwrap_nsurl(url);
  NSBundle* bundle = [NSBundle bundleWithURL:nsurl];
  VALUE  rb_bundle = Qnil;

  if (bundle)
    rb_bundle = wrap_bundle(bundle);

  return rb_bundle;
}

static
VALUE
rb_bundle_info_dict(VALUE self)
{
  NSDictionary* dict = [unwrap_bundle(self) infoDictionary];
  VALUE         hash = Qnil;
  if (dict)
    hash = wrap_dictionary(dict);
  return hash;
}

static
VALUE
rb_bundle_object_for_info_dict_key(VALUE self, VALUE key)
{
  NSString* nskey = unwrap_nsstring(key);
  id obj = [unwrap_bundle(self) objectForInfoDictionaryKey:nskey];
  if (obj)
    return to_ruby(obj);
  return Qnil;
}


static
VALUE
rb_load_plist(VALUE self, VALUE plist_data)
{
  NSData* data = [NSData dataWithBytes:(void*)StringValueCStr(plist_data)
		                length:RSTRING_LEN(plist_data)];
  NSError* err = nil;
  id     plist = [NSPropertyListSerialization propertyListWithData:data
                     	                                   options:0
		                                            format:nil
       	                                                     error:&err];
  [data release];
  if (plist) {
    VALUE list = to_ruby(plist);
    [plist release];
    return list;
  }

  rb_raise(rb_eArgError, "error loading property list: '%s'",
	   [[err localizedDescription] UTF8String]);
  return Qnil; // unreachable
}

VALUE wrap_screen(NSScreen* obj) { WRAP_OBJC(rb_cScreen, NULL); }
VALUE wrap_array_screens(CFArrayRef array) { WRAP_ARRAY(wrap_screen); }
NSScreen* unwrap_screen(VALUE obj) { UNWRAP_OBJC(NSScreen); }

static
VALUE
rb_screen_main(VALUE self)
{
  return wrap_screen([NSScreen mainScreen]);
}

static
VALUE
rb_screen_screens(VALUE self)
{
    return wrap_array_screens((CFArrayRef)[NSScreen screens]);
}

static
VALUE
rb_screen_frame(VALUE self)
{
    return wrap_rect(NSRectToCGRect([unwrap_screen(self) frame]));
}

static
VALUE
rb_screen_wake(VALUE self)
{
  // don't bother if we are awake
  if (!CGDisplayIsAsleep(CGMainDisplayID()))
    return Qtrue;

  if (screen_connection == MACH_PORT_NULL) {
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching(kIOHIDSystemClass));
    if (service != MACH_PORT_NULL) {
      IOServiceOpen(service, mach_task_self(), kIOHIDParamConnectType, &screen_connection);
      IOObjectRelease(service);
    }
    else { // give up
      return Qfalse;
    }
  }

  CGPoint      mouse = NSPointToCGPoint([NSEvent mouseLocation]);
  IOGPoint     point = { mouse.x, mouse.y };
  unsigned int flags = (unsigned int)[NSEvent modifierFlags];
  NXEventData   data;

  IOHIDPostEvent(screen_connection, NX_FLAGSCHANGED, point, &data, kNXEventDataVersion, flags, 0);

  // spin rims while we wait for the screen to fully wake up
  spin(1);

  return Qtrue;
}




// Find and return the dictionary that has the battery info
static
CFDictionaryRef
battery_info()
{
  CFTypeRef  psource_info = IOPSCopyPowerSourcesInfo();
  CFArrayRef     psources = IOPSCopyPowerSourcesList(psource_info);

  // constant global strings (like ruby symbols, or lisp atoms, NXAtom, etc)
  // so we do not need to release it later (unless you really want to)
  CFStringRef type_key    = CFSTR(kIOPSTypeKey);
  CFStringRef battery_key  = CFSTR(kIOPSInternalBatteryType);

  CFIndex length = CFArrayGetCount(psources);
  for (CFIndex i = 0; i < length; i++) {

    CFTypeRef           psource = CFArrayGetValueAtIndex(psources, i);
    CFDictionaryRef source_info = IOPSGetPowerSourceDescription(psource_info, psource);
    CFRetain(source_info);

    if (CFEqual(CFDictionaryGetValue(source_info, type_key), battery_key)) {
      CFRelease(psources);
      CFRelease(psource_info);
      return source_info;
    }
    else {
      CFRelease(source_info);
    }
  }

  CFRelease(psources);
  CFRelease(psource_info);
  return NULL;
}

/*
 * Returns the current battery state
 *
 * The state will be one of:
 *
 *  - `:not_installed`
 *  - `:charged`
 *  - `:charging`
 *  - `:discharging`
 *
 * @return [Symbol]
 */
static
VALUE
rb_battery_state(VALUE self)
{
  // constant global strings (like ruby symbols, or lisp atoms, NXAtom, etc)
  // so we do not need to release it later (unless you really want to)
  CFStringRef charged_key  = CFSTR(kIOPSIsChargedKey);
  CFStringRef charging_key = CFSTR(kIOPSIsChargingKey);

  VALUE              state = battery_not_installed;
  CFDictionaryRef     info = battery_info();

  if (info) {
    if (CFDictionaryGetValue(info, charged_key) == kCFBooleanTrue)
      state = battery_charged;
    else if (CFDictionaryGetValue(info, charging_key) == kCFBooleanTrue)
      state = battery_charging;
    else
      state = battery_discharging;

    CFRelease(info);
  }

  return state;
}

/*
 * Returns the batteries charge level as a percentage from 0 to 1
 *
 * A special value of `-1.0` is returned when there is no battery present.
 *
 * @return [Float]
 */
static
VALUE
rb_battery_level(VALUE self)
{
  // constant global strings (like ruby symbols, or lisp atoms, NXAtom, etc)
  // so we do not need to release it later (unless you really want to)
  CFStringRef capacity_key     = CFSTR(kIOPSCurrentCapacityKey);
  CFStringRef max_capacity_key = CFSTR(kIOPSMaxCapacityKey);

  double         level = -1.0;
  CFDictionaryRef info = battery_info();

  if (info) {
    CFNumberRef current_cap = CFDictionaryGetValue(info, capacity_key);
    CFNumberRef     max_cap = CFDictionaryGetValue(info, max_capacity_key);

    if (current_cap && max_cap) {
      int current = 0;
      int     max = 0;

      CFNumberGetValue(current_cap, kCFNumberIntType, &current);
      CFNumberGetValue(max_cap,     kCFNumberIntType, &max);

      level = ((double)current)/((double)max);
    }

    CFRelease(info);
  }

  return DBL2NUM(level);
}


/*
 * Returns the estimated number of minutes until the battery is fully discharged
 *
 * A special value of `-1` indicates that the value is currently being
 * estimated and you should try again later.
 *
 * A special value of `0` indicates that the battery is not discharging,
 * which usually means that the battery does not exist or is in a
 * charging/charged state.
 *
 * @return [Fixnum]
 */
static
VALUE
rb_battery_time_to_empty(VALUE self)
{
  CFStringRef ttempty_key = CFSTR(kIOPSTimeToEmptyKey);
  int                time = -1;
  CFDictionaryRef    info = battery_info();

  if (info) {
    CFNumberRef current_time = CFDictionaryGetValue(info, ttempty_key);
    if (current_time)
      CFNumberGetValue(current_time, kCFNumberIntType, &time);

    CFRelease(info);
  }

  if (time)
    return INT2FIX(time);
  else
    return INT2FIX(0);
}


/*
 * Returns the estimated number of minutes until the battery is fully charged
 *
 * A special value of `-1` indicates that the value is currently being
 * estimated and you should try again later.
 *
 * @return [Fixnum]
*/
static
VALUE
rb_battery_time_full_charge(VALUE self)
{
  CFStringRef ttfull_key = CFSTR(kIOPSTimeToFullChargeKey);
  int                time = -1;
  CFDictionaryRef    info = battery_info();

  if (info) {
    CFNumberRef current_time = CFDictionaryGetValue(info, ttfull_key);
    if (current_time)
      CFNumberGetValue(current_time, kCFNumberIntType, &time);

    CFRelease(info);
  }

  return INT2FIX(time);
}




void
Init_extras()
{
  Init_bridge();

  // force Ruby to be registered as an app with the system
  [NSApplication sharedApplication];

  /*
   * Document-class: NSRunningApplication
   *
   * A 99% drop-in replacement for Cocoa's `NSWorkspace` class on MRI and other
   * non-MacRuby rubies.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/AppKit/Reference/NSRunningApplication_Class/Reference/Reference.html)
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
  rb_define_method(rb_cRunningApp, "unhide",                 rb_running_app_unhide,            0);
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
  rb_define_method(rb_cRunningApp, "==",                     rb_running_app_equality,          1);

  // Technically these are global constants, but in MacRuby you can still access them from any
  // namespace. So, we will try by first adding them to the NSRunningApplication namespae only
#define RUNNING_APP_CONST(str, val) rb_define_const(rb_cRunningApp, str, INT2FIX(val));
  if (!rb_const_defined_at(rb_cRunningApp, rb_intern("NSApplicationActivateAllWindows"))) {
    RUNNING_APP_CONST("NSApplicationActivateAllWindows",        NSApplicationActivateAllWindows);
    RUNNING_APP_CONST("NSApplicationActivateIgnoringOtherApps", NSApplicationActivateIgnoringOtherApps);

    RUNNING_APP_CONST("NSBundleExecutableArchitectureI386",     NSBundleExecutableArchitectureI386);
    RUNNING_APP_CONST("NSBundleExecutableArchitecturePPC",      NSBundleExecutableArchitecturePPC);
    RUNNING_APP_CONST("NSBundleExecutableArchitectureX86_64",   NSBundleExecutableArchitectureX86_64);
    RUNNING_APP_CONST("NSBundleExecutableArchitecturePPC64",    NSBundleExecutableArchitecturePPC64);
  }


  /*
   * Document-class: NSWorkspace
   *
   * A subset of Cocoa's `NSWorkspace` class.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/ApplicationKit/Classes/NSWorkspace_Class/Reference/Reference.html)
   * for documentation on the methods available in this class.
   */
  rb_cWorkspace = rb_define_class("NSWorkspace", rb_cObject);

  rb_define_singleton_method(rb_cWorkspace, "runningApplications",             rb_workspace_running_apps,   0);
  rb_define_singleton_method(rb_cWorkspace, "sharedWorkspace",                 rb_workspace_shared,         0);
  rb_define_singleton_method(rb_cWorkspace, "frontmostApplication",            rb_workspace_frontmost_app,  0);
  rb_define_singleton_method(rb_cWorkspace, "menuBarOwningApplication",        rb_workspace_menu_bar_owner, 0);
  rb_define_singleton_method(rb_cWorkspace, "showSearchResultsForQueryString", rb_workspace_find,           1);
  rb_define_singleton_method(rb_cWorkspace, "launchAppWithBundleIdentifier",   rb_workspace_launch,         2);

  key_opts         = ID2SYM(rb_intern("options"));
  //  key_event_params = ID2SYM(rb_intern("additionalEventParamDescriptor"));
  //  key_launch_id    = ID2SYM(rb_intern("launchIdentifier"));

#define WORKSPACE_CONST(str, val) rb_define_const(rb_cWorkspace, str, INT2FIX(val));
  if (!rb_const_defined_at(rb_cWorkspace, rb_intern("NSWorkspaceLaunchAndPrint"))) {
    WORKSPACE_CONST("NSWorkspaceLaunchAndPrint",                 NSWorkspaceLaunchAndPrint);
    WORKSPACE_CONST("NSWorkspaceLaunchInhibitingBackgroundOnly", NSWorkspaceLaunchInhibitingBackgroundOnly);
    WORKSPACE_CONST("NSWorkspaceLaunchWithoutAddingToRecents",   NSWorkspaceLaunchWithoutAddingToRecents);
    WORKSPACE_CONST("NSWorkspaceLaunchWithoutActivation",        NSWorkspaceLaunchWithoutActivation);
    WORKSPACE_CONST("NSWorkspaceLaunchAsync",                    NSWorkspaceLaunchAsync);
    //Classic workspace settings deprecated in 10.11 
#ifndef MAC_OS_X_VERSION_10_11
    WORKSPACE_CONST("NSWorkspaceLaunchAllowingClassicStartup",   NSWorkspaceLaunchAllowingClassicStartup);
    WORKSPACE_CONST("NSWorkspaceLaunchPreferringClassic",        NSWorkspaceLaunchPreferringClassic);
#endif
    WORKSPACE_CONST("NSWorkspaceLaunchNewInstance",              NSWorkspaceLaunchNewInstance);
    WORKSPACE_CONST("NSWorkspaceLaunchAndHide",                  NSWorkspaceLaunchAndHide);
    WORKSPACE_CONST("NSWorkspaceLaunchAndHideOthers",            NSWorkspaceLaunchAndHideOthers);
    WORKSPACE_CONST("NSWorkspaceLaunchDefault",                  NSWorkspaceLaunchDefault);
  }


  /*
   * Document-class: NSProcessInfo
   *
   * A subset of Cocoa's `NSProcessInfo` class. Methods that might be
   * useful to have been bridged.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSProcessInfo_Class/Reference/Reference.html)
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
   * useful to have have been bridged.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSHost_Class/Reference/Reference.html)
   * for documentation on the methods available in this class.
   */
  rb_cHost = rb_define_class("NSHost", rb_cObject);

  rb_define_singleton_method(rb_cHost, "currentHost",   rb_host_self,           0);
  rb_define_singleton_method(rb_cHost, "names",         rb_host_names,          0);
  rb_define_singleton_method(rb_cHost, "addresses",     rb_host_addresses,      0);
  rb_define_singleton_method(rb_cHost, "localizedName", rb_host_localized_name, 0);


  /*
   * Document-class: NSBundle
   *
   * A tiny subset of Cocoa's `NSBundle` class. Methods that might be
   * useful to have have been bridged.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSBundle_Class/Reference/Reference.html)
   * for documentation on the methods available in this class.
   */
  rb_cBundle = rb_define_class("NSBundle", rb_cObject);
  rb_define_singleton_method(rb_cBundle, "bundleWithURL", rb_bundle_with_url, 1);
  rb_define_method(rb_cBundle, "infoDictionary",             rb_bundle_info_dict,                0);
  rb_define_method(rb_cBundle, "objectForInfoDictionaryKey", rb_bundle_object_for_info_dict_key, 1);

  rb_define_method(rb_cObject, "load_plist", rb_load_plist, 1);


  /*
   * Document-class: NSScreen
   *
   * A small subset of Cocoa's `NSScreen` class. Methods that might be
   * useful to have have been bridged.
   *
   * See [Apple's Developer Reference](https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/Foundation/Classes/NSScreen_Class/Reference/Reference.html)
   * for documentation on the methods available in this class.
   */
  rb_cScreen = rb_define_class("NSScreen", rb_cObject);

  rb_define_singleton_method(rb_cScreen, "mainScreen", rb_screen_main,    0);
  rb_define_singleton_method(rb_cScreen, "screens",    rb_screen_screens, 0);
  rb_define_singleton_method(rb_cScreen, "wakeup",     rb_screen_wake,    0); // our custom method
  rb_define_method(          rb_cScreen, "frame",      rb_screen_frame,   0);

  /*
   * Document-module: Battery
   *
   * Utility methods for getting information about the system battery
   * (if present).
   */
  rb_mBattery = rb_define_module("Battery");
  rb_extend_object(rb_mBattery, rb_mBattery);

  battery_not_installed = ID2SYM(rb_intern("not_installed"));
  battery_charged       = ID2SYM(rb_intern("charged"));
  battery_charging      = ID2SYM(rb_intern("charging"));
  battery_discharging   = ID2SYM(rb_intern("discharging"));

  rb_define_method(rb_mBattery, "state",     rb_battery_state, 0);
  rb_define_method(rb_mBattery, "level",     rb_battery_level, 0);
  rb_define_method(rb_mBattery, "time_to_discharged", rb_battery_time_to_empty, 0);
  rb_define_method(rb_mBattery, "time_to_charged", rb_battery_time_full_charge, 0);

  rb_define_alias(rb_mBattery, "charge_level", "level");
  rb_define_alias(rb_mBattery, "time_to_empty", "time_to_discharged");
  rb_define_alias(rb_mBattery, "time_to_full_charge", "time_to_charged");
}
