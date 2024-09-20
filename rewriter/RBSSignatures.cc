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
#include "rbs/MethodTypeTranslator.h"
#include "rbs/RBSParser.h"
#include "rewriter/rewriter.h"

using namespace std;

namespace sorbet::rewriter {

class RBSSignaturesWalk {
    // TODO: review and clean up
    rbs::MethodComments findRBSSignatures(string_view sourceCode, core::LocOffsets loc) {
        std::vector<rbs::RBSAnnotation> annotations;
        std::vector<rbs::RBSSignature> signatures;

        uint32_t beginIndex = loc.beginPos();

        // Everything in the file before the method definition
        string_view preDefinition = sourceCode.substr(0, sourceCode.rfind('\n', beginIndex));

        // Get all the lines before it
        std::vector<string_view> all_lines = absl::StrSplit(preDefinition, '\n');

        // We compute the current position in the source so we know the location of each comment
        uint32_t index = beginIndex;

        // Iterate from the last line, to the first line
        for (auto it = all_lines.rbegin(); it != all_lines.rend(); it++) {
            // std::cout << "line: '" << *it << "' size: " << it->size() << std::endl;
            index -= it->size();
            index -= 1;

            string_view line = absl::StripAsciiWhitespace(*it);

            // Short circuit when line is empty
            if (line.empty()) {
                break;
            }

            // Handle single-line sig block
            else if (absl::StartsWith(line, "sig")) {
                // Do nothing for a one-line sig block
                // TODO: Handle single-line sig blocks
            }

            // Handle multi-line sig block
            else if (absl::StartsWith(line, "end")) {
                // ASSUMPTION: We either hit the start of file, a `sig do`/`sig(:final) do` or an `end`
                // TODO: Handle multi-line sig blocks
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

            // Handle a RBS sig annotation `#: SomeRBS`
            else if (absl::StartsWith(line, "#:")) {
                // Account for whitespace before comment e.g
                // # abc -> "abc"
                // #abc -> "abc"
                // int skip_after_hash = absl::StartsWith(line, "#: ") ? 3 : 2;

                int lineSize = line.size();
                auto rbsSignature = rbs::RBSSignature{
                    core::LocOffsets{index, index + lineSize},
                    line.substr(line.find("#:") + 2),
                };
                signatures.insert(signatures.begin(), rbsSignature);
            }

            // Handle RDoc annotations `# @abstract`
            else if (absl::StartsWith(line, "# @")) {
                int lineSize = line.size();
                auto annotation = rbs::RBSAnnotation{
                    core::LocOffsets{index, index + lineSize},
                    line.substr(line.find("# ") + 2),
                };
                annotations.insert(annotations.begin(), annotation);
            }

            // Ignore other comments
            else if (absl::StartsWith(line, "#")) {
                continue;
            }

            // No other cases applied to this line, so stop looking.
            else {
                break;
            }
        }

        // std::cout << "documentation_lines: " << documentation_lines.size() << std::endl;
        // string documentation = absl::StrJoin(documentation_lines.rbegin(), documentation_lines.rend(), "\n");
        // std::cout << "documentation: '" << documentation << "'" << std::endl;
        // int documentationSize = documentation.size();

        // Remove trailing whitespace from the documentation
        // string_view stripped = absl::StripTrailingAsciiWhitespace(documentation);
        // if (stripped.size() != documentation.size()) {
        //     documentation.resize(stripped.size());
        // }

        return rbs::MethodComments{annotations, signatures};
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

            auto methodLoc = methodDef->loc;
            auto methodComments = findRBSSignatures(ctx.file.data(ctx).source(), methodLoc);

            for (auto &signature : methodComments.signatures) {
                auto docLoc = signature.loc;
                auto doc = signature.signature;
                // std::cout << "docLoc: " << docLoc.beginPos() << std::endl;
                // std::cout << "doc: '" << doc << "'" << std::endl;

                auto rbsMethodType = rbs::RBSParser::parseSignature(ctx, docLoc, methodLoc, doc);

                if (rbsMethodType != Qnil) {
                    auto sig = rbs::MethodTypeTranslator::toRBI(ctx, docLoc, methodDef, rbsMethodType,
                                                                methodComments.annotations);
                    classDef->rhs.emplace_back(std::move(sig));
                }
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
