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

// @kaan: Can't create a map because ExpressionPtr is a reference
// @kaan: Comments& instead?
// @kaan: This is a vector of pairs of the line number and the comments for that line
vector<pair<int, Comments>> commentMapping;

class RBSSignaturesWalk {
    void findRBSComments(string_view sourceCode) {
        vector<rbs::Comment> annotations;
        vector<rbs::Comment> signatures;

        auto lines = absl::StrSplit(sourceCode, '\n');
        for (int i = 0; i < lines.size(); i++) {
            auto &line = lines[i];
            if (absl::StartsWith(line, "#:")) {
                signatures.emplace_back(line);
            } else if (absl::StartsWith(line, "@")) {
                annotations.emplace_back(line);
            }

            commentMapping.emplace_back(i, Comments{annotations, signatures});
        }
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
    }

public:
    RBSSignaturesWalk(core::MutableContext ctx) {}

    void preTransformClassDef(core::MutableContext ctx, ast::ExpressionPtr &tree) {
        auto &classDef = ast::cast_tree_nonnull<ast::ClassDef>(tree);

        auto newRHS = ast::ClassDef::RHS_store();
        newRHS.reserve(classDef.rhs.size());

        for (auto &stat : classDef.rhs) {
            if (auto methodDef = ast::cast_tree<ast::MethodDef>(stat)) {
                transformMethodDef(ctx, newRHS, methodDef);
            } else if (auto send = ast::cast_tree<ast::Send>(stat)) {
                if (isAccessor(send)) {
                    transformAccessor(ctx, newRHS, send);
                } else if (auto methodDef = asVisibilityWrappedMethod(send)) {
                    transformMethodDef(ctx, newRHS, methodDef);
                }
            }

            newRHS.emplace_back(move(stat));
        }

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
