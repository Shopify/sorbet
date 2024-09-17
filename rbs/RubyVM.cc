#include "RubyVM.h"
#include "rbs_common.h"

namespace sorbet::rbs {

void RubyVM::initialize() {
    RUBY_INIT_STACK;

    ruby_init();
    ruby_init_loadpath();

    // TODO: read this from the dep
    VALUE rb_load_path = rb_gv_get("$:");
    VALUE new_path = rb_str_new_cstr("/Users/at/src/github.com/ruby/rbs/lib");
    rb_funcall(rb_load_path, rb_intern("unshift"), 1, new_path);

    rb_require("rbs.rb");

    rbs__init_constants();
    // rbs__init_location(); // TODO: do we need this?
    // rbs__init_parser(); // TODO: do we need this?
}

void RubyVM::cleanup() {
    ruby_cleanup(0);
}

} // namespace sorbet::rbs
