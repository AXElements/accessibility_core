#include "ruby.h"

static VALUE rb_mAccessibility;
static VALUE rb_mCore;

void
Init_caccessibility()
{
  rb_mAccessibility = rb_define_module("Accessibility");
  rb_mCore          = rb_define_module_under(rb_mAccessibility, "Core");
}
