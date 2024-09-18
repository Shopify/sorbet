#include "RBSParser.h"
#include "ast/ast.h"
#include "ast/Helpers.h"
#include "core/Names.h"
#include "core/errors/rewriter.h"
#include "core/GlobalState.h"
#include <cstring>
#include <functional>

namespace sorbet::rbs {

VALUE RBSParser::parse_method_type_wrapper(VALUE string) {
    VALUE cIO = rb_const_get(rb_cObject, rb_intern("RBS"));
    VALUE cBuffer = rb_const_get(cIO, rb_intern("Buffer"));

    VALUE kwargs = rb_hash_new();
    rb_hash_aset(kwargs, ID2SYM(rb_intern("name")), rb_str_new2("(string)"));
    rb_hash_aset(kwargs, ID2SYM(rb_intern("content")), string);
    VALUE argv[1] = {kwargs};
    VALUE buffer = rb_funcallv_kw(cBuffer, rb_intern("new"), 1, argv, RB_PASS_KEYWORDS);

    int length = RSTRING_LEN(string);
    lexstate *lexer = alloc_lexer(string, 0, length);
    parserstate *parser = alloc_parser(buffer, lexer, 0, length, Qnil);

    return parse_method_type(parser);
}

VALUE RBSParser::parseRBS(core::MutableContext ctx, const std::string& docString, sorbet::core::LocOffsets loc) {
    VALUE string = rb_str_new2(docString.c_str());

    int state;
    VALUE rbsMethodType = rb_protect(parse_method_type_wrapper, string, &state);

    if (state) {
        // An exception occurred
        VALUE exception = rb_errinfo();
        rb_set_errinfo(Qnil);  // Clear the error info

        // Get the error message from the exception
        VALUE errorMessage = rb_funcall(exception, rb_intern("message"), 0);
        char* cErrorMessage = StringValueCStr(errorMessage);

        // Get the backtrace
        VALUE backtrace = rb_funcall(exception, rb_intern("backtrace"), 0);

        // Log the error or handle it as needed
        if (auto e = ctx.beginError(loc, core::errors::Rewriter::RBSError)) {
            e.setHeader("Failed to parse RBS signature: {}", cErrorMessage);

            rb_p(exception);
            rb_p(backtrace);
            // e.addErrorNote("Stack trace: {}", rb_ary_join(backtrace, rb_str_new2("\n")));
        }

        return Qnil;
    }

    return rbsMethodType;
}

} // namespace sorbet::rbs
