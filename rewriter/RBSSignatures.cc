#include "rewriter/RBSSignatures.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "ast/treemap/treemap.h"
#include "core/errors/rewriter.h"
#include "rbs/rbs_common.h"
#include "rbs/MethodTypeTranslator.h"
#include "rewriter/rewriter.h"

using namespace std;

namespace sorbet::rewriter {

class RBSSignaturesWalk {
    // TODO: review and clean up
    optional<string> findDocumentation(string_view sourceCode, int beginIndex) {
        // Everything in the file before the method definition.
        string_view preDefinition = sourceCode.substr(0, sourceCode.rfind('\n', beginIndex));

        // Get all the lines before it.
        std::vector<string_view> all_lines = absl::StrSplit(preDefinition, '\n');

        // if there are no lines before the method definition, we're at the top of the file.
        if (all_lines.empty()) {
            return nullopt;
        }

        std::vector<string_view> documentation_lines;

        // Iterate from the last line, to the first line
        for (auto it = all_lines.rbegin(); it != all_lines.rend(); it++) {
            string_view line = absl::StripAsciiWhitespace(*it);

            // Short circuit when line is empty
            if (line.empty()) {
                break;
            }

            // Handle single-line sig block
            else if (absl::StartsWith(line, "sig")) {
                // Do nothing for a one-line sig block
            }

            // Handle multi-line sig block
            else if (absl::StartsWith(line, "end")) {
                // ASSUMPTION: We either hit the start of file, a `sig do`/`sig(:final) do` or an `end`
                it++;
                while (
                    // SOF
                    it != all_lines.rend()
                    // Start of sig block
                    && !(absl::StartsWith(absl::StripAsciiWhitespace(*it), "sig do") ||
                         absl::StartsWith(absl::StripAsciiWhitespace(*it), "sig(:final) do"))
                    // Invalid end keyword
                    && !absl::StartsWith(absl::StripAsciiWhitespace(*it), "end")) {
                    it++;
                };

                // We have either
                // 1) Reached the start of the file
                // 2) Found a `sig do`
                // 3) Found an invalid end keyword
                if (it == all_lines.rend() || absl::StartsWith(absl::StripAsciiWhitespace(*it), "end")) {
                    break;
                }

                // Reached a sig block.
                line = absl::StripAsciiWhitespace(*it);
                ENFORCE(absl::StartsWith(line, "sig do") || absl::StartsWith(line, "sig(:final) do"));

                // Stop looking if this is a single-line block e.g `sig do; <block>; end`
                if ((absl::StartsWith(line, "sig do;") || absl::StartsWith(line, "sig(:final) do;")) &&
                    absl::EndsWith(line, "end")) {
                    break;
                }

                // Else, this is a valid sig block. Move on to any possible documentation.
            }

            // Handle a comment line. Do not count typing declarations.
            else if (absl::StartsWith(line, "#: ")) {
                // Account for whitespace before comment e.g
                // # abc -> "abc"
                // #abc -> "abc"
                int skip_after_hash = absl::StartsWith(line, "#: ") ? 3 : 2;

                string_view comment = line.substr(line.find("#: ") + skip_after_hash);

                documentation_lines.emplace_back(comment);

                // Account for yarddoc lines by inserting an extra newline right before
                // the yarddoc line (note that we are reverse iterating)
                if (absl::StartsWith(comment, "@")) {
                    documentation_lines.emplace_back("");
                }
            }

            // No other cases applied to this line, so stop looking.
            else {
                break;
            }
        }

        string documentation = absl::StrJoin(documentation_lines.rbegin(), documentation_lines.rend(), "\n");
        string_view stripped = absl::StripTrailingAsciiWhitespace(documentation);
        if (stripped.size() != documentation.size()) {
            documentation.resize(stripped.size());
        }

        if (documentation.empty()) {
            return nullopt;
        } else {
            return documentation;
        }
    }

    static VALUE parse_method_type_wrapper(VALUE string) {
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

public:
    RBSSignaturesWalk(core::MutableContext ctx) {}

    void preTransformClassDef(core::MutableContext ctx, ast::ExpressionPtr &tree) {
        auto *classDef = ast::cast_tree<ast::ClassDef>(tree);
        if (!classDef) {
            return;
        }

        auto oldRHS = std::move(classDef->rhs);
        classDef->rhs.clear();
        classDef->rhs.reserve(oldRHS.size());

        // if (auto e = ctx.beginError(classDef->loc, core::errors::Rewriter::RBSError)) {
        //     e.setHeader("`{}` error", "class << EXPRESSION");
        // }

        for (auto &stat : oldRHS) {
            auto *methodDef = ast::cast_tree<ast::MethodDef>(stat);
            if (methodDef == nullptr) {
                classDef->rhs.emplace_back(std::move(stat));
                continue;
            }

            // std::cout << "method: " << methodDef->name.show(ctx) << std::endl;

            vector<string> documentation;
            auto loc = methodDef->loc;
            auto doc = findDocumentation(ctx.file.data(ctx).source(), loc.beginPos());
            if (doc.has_value() && !doc->empty()) {
                documentation.emplace_back(*doc);
            }

            if (documentation.empty()) {
                classDef->rhs.emplace_back(std::move(stat));
                continue;
            }

            optional<string> docString;
            if (!documentation.empty()) {
                docString = absl::StrJoin(documentation, "\n\n");
                std::cout << "rbs: '" << *docString << "'" << std::endl;
            }

            VALUE string = rb_str_new2(docString->c_str());
            // StringValue(string);

            int state;
            VALUE rbsMethodType = Qnil;
            rbsMethodType = rb_protect(parse_method_type_wrapper, string, &state);

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
                if (auto e = ctx.beginError(methodDef->loc, core::errors::Rewriter::RBSError)) {
                    e.setHeader("Failed to parse RBS signature: {}", cErrorMessage);

                    rb_p(exception);
                    rb_p(backtrace);
                    // e.addErrorNote("Stack trace: {}", rb_ary_join(backtrace, rb_str_new2("\n")));
                }

                // Skip adding the signature for this method
                classDef->rhs.emplace_back(std::move(stat));
                continue;
            }

            // Only proceed with signature translation if parsing was successful
            if (rbsMethodType != Qnil) {
                auto sig = sorbet::rbs::MethodTypeTranslator::toRBI(ctx, methodDef, rbsMethodType);
                classDef->rhs.emplace_back(std::move(sig));
            }

            classDef->rhs.emplace_back(std::move(stat));
        }
    }
};

ast::ExpressionPtr RBSSignatures::run(core::MutableContext ctx, ast::ExpressionPtr tree) {
    RBSSignaturesWalk rbs_translate(ctx);
    ast::TreeWalk::apply(ctx, rbs_translate, tree);

    return tree;
}

}; // namespace sorbet::rewriter
