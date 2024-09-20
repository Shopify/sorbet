#include "RubyVM.h"
#include "rbs_common.h"
#include <iostream>

namespace sorbet::rbs {

static VALUE require_rbs(VALUE) {
    rb_require("rbs.rb");
    return Qnil;
}

static VALUE init_constants(VALUE) {
    rbs__init_constants();
    return Qnil;
}

static VALUE init_location(VALUE) {
    rbs__init_location();
    return Qnil;
}

void RubyVM::initialize() {
    RUBY_INIT_STACK;

    ruby_init();
    ruby_init_loadpath();

    // TODO: read this from the dep
    VALUE rb_load_path = rb_gv_get("$:");
    VALUE new_path = rb_str_new_cstr("/Users/at/src/github.com/ruby/rbs/lib");
    rb_funcall(rb_load_path, rb_intern("unshift"), 1, new_path);

    int state;

    rb_protect(require_rbs, Qnil, &state);
    if (state) {
        handle_ruby_exception("Failed to require rbs.rb", state);
        return;
    }

    rb_protect(init_constants, Qnil, &state);
    if (state) {
        handle_ruby_exception("Failed to initialize RBS constants", state);
        return;
    }

    rb_protect(init_location, Qnil, &state);
    if (state) {
        handle_ruby_exception("Failed to initialize RBS location", state);
        return;
    }
}

void RubyVM::cleanup() {
    ruby_cleanup(0);
}

void RubyVM::handle_ruby_exception(const char *context, int state) {
    VALUE exception = rb_errinfo();
    rb_set_errinfo(Qnil); // Clear the error info

    VALUE errorMessage = rb_funcall(exception, rb_intern("message"), 0);
    rb_p(errorMessage);
    VALUE backtrace = rb_funcall(exception, rb_intern("backtrace"), 0);
    rb_p(backtrace);
}

} // namespace sorbet::rbs
