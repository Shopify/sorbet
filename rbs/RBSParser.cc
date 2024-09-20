#include "RBSParser.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "core/GlobalState.h"
#include "core/Names.h"
#include "core/errors/rewriter.h"
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

VALUE RBSParser::parseSignature(core::MutableContext ctx, sorbet::core::LocOffsets docLoc,
                                sorbet::core::LocOffsets methodLoc, const std::string_view docString) {
    // std::cout << "parseRBS" << std::endl;
    VALUE string = rb_str_new(docString.data(), docString.size());

    int state;
    VALUE rbsMethodType = rb_protect(parse_method_type_wrapper, string, &state);

    if (state) {
        // An exception occurred
        VALUE exception = rb_errinfo();
        rb_set_errinfo(Qnil); // Clear the error info

        // Get the error message from the exception
        VALUE errorMessage = rb_funcall(exception, rb_intern("error_message"), 0);
        char *cErrorMessage = StringValueCStr(errorMessage);

        VALUE location = rb_funcall(exception, rb_intern("location"), 0);
        auto startColumn = NUM2INT(rb_funcall(location, rb_intern("start_column"), 0));
        auto endColumn = NUM2INT(rb_funcall(location, rb_intern("end_column"), 0));
        // std::cout << "docLoc: " << docLoc.showRaw(ctx) << (docLoc.beginPos()) << std::endl;

        // Get the backtrace
        // VALUE backtrace = rb_funcall(exception, rb_intern("backtrace"), 0);

        // Log the error or handle it as needed
        core::LocOffsets offset{docLoc.beginPos() + startColumn + 2, docLoc.beginPos() + endColumn + 2};
        // std::cout << "offset: " << offset.showRaw(ctx) << (offset.beginPos()) << std::endl;
        if (auto e = ctx.beginError(offset, core::errors::Rewriter::RBSError)) {
            e.setHeader("Failed to parse RBS signature ({})", cErrorMessage);

            // rb_p(exception);
            // rb_p(backtrace);
            // e.addErrorNote("Stack trace: {}", rb_ary_join(backtrace, rb_str_new2("\n")));
        }

        return Qnil;
    }

    return rbsMethodType;
}

} // namespace sorbet::rbs
