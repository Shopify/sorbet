#include "common/common.h"
#include "main/options/options.h"
#include "main/realmain.h"

#include "ruby.h"

extern "C" {
#include "rbs_parser/constants.h"
#include "rbs_parser/location.h"
#include "rbs_parser/parser.h"
}

int main(int argc, char *argv[]) {
    ruby_init();
    ruby_init_loadpath();

    VALUE rb_load_path = rb_gv_get("$:");
    VALUE new_path = rb_str_new_cstr("/Users/at/src/github.com/ruby/rbs/lib");
    rb_funcall(rb_load_path, rb_intern("unshift"), 1, new_path);

    rb_require("rbs.rb");

    rbs__init_constants();
    // rbs__init_location();
    // rbs__init_parser();

    try {
        return sorbet::realmain::realmain(argc, argv);
    } catch (sorbet::EarlyReturnWithCode &c) {
        return c.returnCode;
    }
};
