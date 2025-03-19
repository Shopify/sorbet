#include "rewriter/RBSSignatures.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "ast/Helpers.h"
#include "ast/treemap/treemap.h"
#include "core/errors/rewriter.h"
#include "rbs/MethodTypeTranslator.h"
#include "rbs/RBSParser.h"
#include "rbs/TypeTranslator.h"
#include <optional>

using namespace std;

namespace sorbet::rewriter {

namespace {

/**
 * A collection of annotation and signature comments found on a method definition.
 */
struct Comments {
    /**
     * RBS annotation comments found on a method definition.
     *
     * Annotations are formatted as `@some_annotation`.
     */
    vector<rbs::Comment> annotations;

    /**
     * RBS signature comments found on a method definition.
     *
     * Signatures are formatted as `#: () -> void`.
     */
    vector<rbs::Comment> signatures;
};

/**
 * Strip ASCII whitespace from the beginning and end of a string_view, but keep track of
 * how many whitespace characters were removed from the beginning.
 */
string_view StripAsciiWhitespaceWithCount(string_view str, uint32_t &prefixWhitespaceCount) {
    prefixWhitespaceCount = 0;
    size_t start = 0;
    while (start < str.size() && absl::ascii_isspace(str[start])) {
        ++start;
        ++prefixWhitespaceCount;
    }

    size_t end = str.size();
    while (end > start && absl::ascii_isspace(str[end - 1])) {
        --end;
    }

    return str.substr(start, end - start);
}

class RBSSignaturesWalk {
    uint32_t prevExprLoc = core::INVALID_POS_LOC;

    Comments findRBSComments(string_view sourceCode, core::LocOffsets loc) {
        vector<rbs::Comment> annotations;
        vector<rbs::Comment> signatures;

        uint32_t beginIndex = loc.beginPos();
        uint32_t endIndex = prevExprLoc;
        if (prevExprLoc == core::INVALID_POS_LOC) {
            endIndex = 0;
        }

        if (beginIndex == endIndex) {
            return Comments{annotations, signatures};
        }

        // We iterate in reverse, so beginIndex > endIndex
        ENFORCE(beginIndex > endIndex);

        // Section we're interested in
        string_view preDefinition = sourceCode.substr(endIndex, beginIndex - endIndex);

        // Get all the lines before it
        vector<string_view> all_lines = absl::StrSplit(preDefinition, '\n');
        // fmt::print("all_lines: {}\n", all_lines);

        // We compute the current position in the source so we know the location of each comment
        uint32_t index = beginIndex;

        // Iterate in reverse
        for (auto it = all_lines.rbegin(); it != all_lines.rend(); it++) {
            // fmt::print("LOOP index: {}, endIndex: {}\n", index, endIndex);
            // fmt::print("it->size(): {}\n", it->size());
            // fmt::print("*it:{}\n", *it);
            if (it->size() == 0) {
                index -= 1;
                continue;
            } else {
                index -= it->size();
            }

            uint32_t leadingWhitespaceCount = 0;
            string_view line = StripAsciiWhitespaceWithCount(*it, leadingWhitespaceCount);
            // fmt::print("leadingWhitespaceCount: {}\n", leadingWhitespaceCount);
            // fmt::print("line:{}\n", line);

            // Short circuit when line is empty
            if (line.empty()) {
                // fmt::print("empty line\n");
                index -= 1;
            }

            // Handle an RBS sig annotation `#: SomeRBS`
            else if (absl::StartsWith(line, "#:")) {
                // Account for whitespace before the annotation e.g
                // #: abc -> "abc"
                // #:abc -> "abc"
                // fmt::print("encountered rbs signature\n");
                int lineSize = line.size();
                // fmt::print("index + leadingWhitespaceCount: {}, index + leadingWhitespaceCount + lineSize: {}\n",
                //            index + leadingWhitespaceCount, index + leadingWhitespaceCount + lineSize);
                auto rbsSignature = rbs::Comment{
                    core::LocOffsets{index + leadingWhitespaceCount, index + leadingWhitespaceCount + lineSize},
                    line.substr(2),
                };
                signatures.emplace_back(rbsSignature);
                index -= 1;
            }

            // Handle RDoc annotations `# @abstract`
            else if (absl::StartsWith(line, "# @")) {
                int lineSize = line.size();
                auto annotation = rbs::Comment{
                    core::LocOffsets{index + leadingWhitespaceCount, index + leadingWhitespaceCount + lineSize},
                    line.substr(3),
                };
                // fmt::print("encountered rdoc annotation\n");
                annotations.emplace_back(annotation);
                index -= 1;
            }

            // Ignore other comments
            else if (absl::StartsWith(line, "#")) {
                // fmt::print("encountered other comment\n");
                index -= 1;
            }

            // Not interested in this line, e.g `class Foo`
            else {
                index -= 1;
            }
        }

        reverse(annotations.begin(), annotations.end());
        reverse(signatures.begin(), signatures.end());

        return Comments{annotations, signatures};
    }

    // Check if the send is an accessor e.g `attr_reader`, `attr_writer`, `attr_accessor`
    bool isAccessor(ast::Send *send) {
        if (!send->recv.isSelfReference()) {
            return false;
        }

        core::NameRef name = send->fun;
        return name == core::Names::attrReader() || name == core::Names::attrWriter() ||
               name == core::Names::attrAccessor();
    }

    // Check if the send is a visibility modifier e.g `public`, `protected`, `private` before a method definition
    // and return the method definition if it is
    ast::MethodDef *asVisibilityWrappedMethod(ast::Send *send) {
        if (!send->recv.isSelfReference()) {
            return nullptr;
        }

        if (send->posArgs().size() != 1) {
            return nullptr;
        }

        if (!ast::cast_tree<ast::MethodDef>(send->getPosArg(0))) {
            return nullptr;
        }

        core::NameRef name = send->fun;
        if (name == core::Names::public_() || name == core::Names::protected_() || name == core::Names::private_() ||
            name == core::Names::privateClassMethod() || name == core::Names::publicClassMethod() ||
            name == core::Names::packagePrivate() || name == core::Names::packagePrivateClassMethod()) {
            return ast::cast_tree<ast::MethodDef>(send->getPosArg(0));
        }

        return nullptr;
    }

    void transformMethodDef(core::MutableContext ctx, ast::ClassDef::RHS_store &newRHS, ast::MethodDef *methodDef) {
        auto methodComments = findRBSComments(ctx.file.data(ctx).source(), methodDef->loc);

        for (auto &signature : methodComments.signatures) {
            auto rbsMethodType = rbs::RBSParser::parseSignature(ctx, signature);
            if (rbsMethodType.first) {
                auto sig = rbs::MethodTypeTranslator::methodSignature(ctx, methodDef, move(rbsMethodType.first.value()),
                                                                      methodComments.annotations);
                newRHS.emplace_back(move(sig));
            } else {
                ENFORCE(rbsMethodType.second);
                if (auto e = ctx.beginError(rbsMethodType.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
                    e.setHeader("Failed to parse RBS signature ({})", rbsMethodType.second->message);
                }
            }
        }
        prevExprLoc = methodDef->loc.endPos();
    }

    void transformAccessor(core::MutableContext ctx, ast::ClassDef::RHS_store &newRHS, ast::Send *send) {
        auto attrComments = findRBSComments(ctx.file.data(ctx).source(), send->loc);

        for (auto &signature : attrComments.signatures) {
            auto rbsType = rbs::RBSParser::parseType(ctx, signature);
            if (rbsType.first) {
                auto sig = rbs::MethodTypeTranslator::attrSignature(ctx, send, move(rbsType.first.value()),
                                                                    attrComments.annotations);
                newRHS.emplace_back(move(sig));
            } else {
                ENFORCE(rbsType.second);

                // Before raising a parse error, let's check if the user tried to use a method signature on an accessor
                auto rbsMethodType = rbs::RBSParser::parseSignature(ctx, signature);
                if (rbsMethodType.first) {
                    if (auto e = ctx.beginError(rbsType.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
                        e.setHeader("Using a method signature on an accessor is not allowed, use a bare type instead");
                    }
                } else {
                    if (auto e = ctx.beginError(rbsType.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
                        e.setHeader("Failed to parse RBS type ({})", rbsType.second->message);
                    }
                }
            }
        }
        prevExprLoc = send->loc.endPos();
    }

    void transformRHS(core::MutableContext ctx, ast::ClassDef::RHS_store &newRHS, ast::ClassDef::RHS_store &oldRHS) {
        for (auto &stat : oldRHS) {
            // Make a copy of the original stat before we potentially modify it
            ast::ExpressionPtr statCopy;

            if (auto classDef = ast::cast_tree<ast::ClassDef>(stat)) {
                // Process nested class definitions by recursively transforming their RHS
                auto nestedNewRHS = ast::ClassDef::RHS_store();
                nestedNewRHS.reserve(classDef->rhs.size());

                transformRHS(ctx, nestedNewRHS, classDef->rhs);
                classDef->rhs = move(nestedNewRHS);
                prevExprLoc = classDef->loc.endPos();

                statCopy = move(stat);
            } else if (auto methodDef = ast::cast_tree<ast::MethodDef>(stat)) {
                transformMethodDef(ctx, newRHS, methodDef);
                statCopy = move(stat);
            } else if (auto send = ast::cast_tree<ast::Send>(stat)) {
                if (isAccessor(send)) {
                    transformAccessor(ctx, newRHS, send);
                    statCopy = move(stat);
                } else if (auto methodDef = asVisibilityWrappedMethod(send)) {
                    transformMethodDef(ctx, newRHS, methodDef);
                    statCopy = move(stat);
                }
            }

            if (statCopy == nullptr) {
                statCopy = move(stat);
            }

            newRHS.emplace_back(move(statCopy));
        }
    }

public:
    RBSSignaturesWalk(core::MutableContext ctx) {}

    void preTransformClassDef(core::MutableContext ctx, ast::ExpressionPtr &tree) {
        auto &classDef = ast::cast_tree_nonnull<ast::ClassDef>(tree);

        // Root classes iterate over their nested classes
        if (classDef.symbol != core::Symbols::root()) {
            return;
        }

        prevExprLoc = core::INVALID_POS_LOC;

        auto newRHS = ast::ClassDef::RHS_store();
        newRHS.reserve(classDef.rhs.size());

        transformRHS(ctx, newRHS, classDef.rhs);

        classDef.rhs = move(newRHS);
    }
};

} // namespace

ast::ExpressionPtr RBSSignatures::run(core::MutableContext ctx, ast::ExpressionPtr tree) {
    RBSSignaturesWalk rbsTranslate(ctx);
    ast::TreeWalk::apply(ctx, rbsTranslate, tree);

    return tree;
}

}; // namespace sorbet::rewriter
