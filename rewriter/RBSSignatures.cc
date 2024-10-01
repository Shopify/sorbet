#include "rewriter/RBSSignatures.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "ast/Helpers.h"
#include "ast/ast.h"
#include "ast/treemap/treemap.h"
#include "core/GlobalState.h"
#include "core/errors/rewriter.h"
#include "rbs/MethodTypeTranslator.h"
#include "rbs/RBSParser.h"
#include "rbs/TypeTranslator.h"
#include "rewriter/rewriter.h"
#include <optional>

using namespace std;

namespace sorbet::rewriter {

class RBSSignaturesWalk {
    // TODO: review and clean up
    rbs::MethodComments findRBSComments(string_view sourceCode, core::LocOffsets loc) {
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

        return rbs::MethodComments{annotations, signatures};
    }

    bool isAttr(ast::ExpressionPtr &recv) {
        auto *send = ast::cast_tree<ast::Send>(recv);
        if (!send) {
            return false;
        }

        if (!send->recv.isSelfReference()) {
            return false;
        }

        core::NameRef name = send->fun;
        if (name != core::Names::attrReader() && name != core::Names::attrWriter() &&
            name != core::Names::attrAccessor()) {
            return false;
        }

        return true;
    }

    std::optional<rbs::RBSInlineAnnotation> getTrailingComment(string_view sourceCode, core::LocOffsets loc) {
        uint32_t beginIndex = loc.beginPos();
        uint32_t endIndex = loc.endPos();

        // Find the start of the line containing the location
        size_t lineStart = sourceCode.rfind('\n', beginIndex);
        lineStart = (lineStart == string_view::npos) ? 0 : lineStart + 1;

        // Find the end of the line containing the location
        size_t lineEnd = sourceCode.find('\n', endIndex);
        lineEnd = (lineEnd == string_view::npos) ? sourceCode.length() : lineEnd;

        // Extract the full line
        string_view fullLine = sourceCode.substr(lineStart, lineEnd - lineStart);

        // Find the position of the first '#'
        size_t firstHashPos = fullLine.find('#');
        if (firstHashPos != string_view::npos) {
            // Check if it's an RBS annotation (either #: or #::)
            if (fullLine.substr(firstHashPos, 2) == "#:" || fullLine.substr(firstHashPos, 3) == "#::") {
                size_t annotationStart = firstHashPos + 2; // Default for #:
                bool isCast = false;
                if (fullLine.substr(firstHashPos, 3) == "#::") {
                    annotationStart = firstHashPos + 3; // Adjust for #::
                    isCast = true;
                }

                // Find the next '#' after the RBS annotation
                size_t nextHashPos = fullLine.find('#', annotationStart);
                string_view rbsAnnotation;
                if (nextHashPos != string_view::npos) {
                    // Extract the RBS type annotation
                    rbsAnnotation = fullLine.substr(annotationStart, nextHashPos - annotationStart);
                } else {
                    // If there's no second '#', take everything after #: or #::
                    rbsAnnotation = fullLine.substr(annotationStart);
                }

                return rbs::RBSInlineAnnotation{
                    core::LocOffsets{static_cast<uint32_t>(lineStart + annotationStart),
                                     static_cast<uint32_t>(lineStart + annotationStart + rbsAnnotation.length())},
                    absl::StripAsciiWhitespace(rbsAnnotation), isCast};
            }
        }

        // Return nullopt if no RBS annotation is found
        return std::nullopt;
    }

    ast::ExpressionPtr makeCast(ast::ExpressionPtr &stat, ast::ExpressionPtr &type, bool isCast) {
        if (isCast) {
            return ast::MK::Cast(stat.loc(), std::move(stat), std::move(type));
        } else {
            return ast::MK::Let(stat.loc(), std::move(stat), std::move(type));
        }
    }

    ast::ExpressionPtr insertCast(core::MutableContext ctx, ast::ExpressionPtr &stat) {
        std::cout << "stat: " << stat.showRaw(ctx) << std::endl;
        std::cout << "stat loc: " << stat.loc().showRaw(ctx) << std::endl;

        auto loc = stat.loc();

        auto trailingComment = getTrailingComment(ctx.file.data(ctx).source(), loc);
        if (!trailingComment) {
            return std::move(stat);
        }

        std::cout << "trailingComment: " << trailingComment->string << std::endl;

        auto docLoc = trailingComment->loc;
        auto doc = trailingComment->string;
        auto rbsType = rbs::RBSParser::parseType(ctx, docLoc, loc, doc);

        if (rbsType == Qnil) {
            return std::move(stat);
        }

        auto type = rbs::TypeTranslator::toRBI(ctx, rbsType, loc);

        if (auto *assign = ast::cast_tree<ast::Assign>(stat)) {
            return makeCast(assign->rhs, type, trailingComment->isCast);
        }

        if (auto *ret = ast::cast_tree<ast::Return>(stat)) {
            return makeCast(ret->expr, type, true);
        }

        return makeCast(stat, type, true);
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

        for (auto &stat : oldRHS) {
            if (auto *methodDef = ast::cast_tree<ast::MethodDef>(stat)) {
                auto methodLoc = methodDef->loc;
                auto methodComments = findRBSComments(ctx.file.data(ctx).source(), methodLoc);

                for (auto &signature : methodComments.signatures) {
                    auto docLoc = signature.loc;
                    auto doc = signature.signature;
                    auto rbsMethodType = rbs::RBSParser::parseSignature(ctx, docLoc, methodLoc, doc);

                    if (rbsMethodType != Qnil) {
                        auto sig = rbs::MethodTypeTranslator::methodSignature(ctx, docLoc, methodDef, rbsMethodType,
                                                                              methodComments.annotations);
                        classDef->rhs.emplace_back(std::move(sig));
                    }
                }

                // std::cout << "methodDef: " << stat.showRaw(ctx) << std::endl;
                if (auto stmts = ast::cast_tree<ast::InsSeq>(methodDef->rhs)) {
                    // auto oldStats = std::move(stmts->stats);
                    // stmts->stats.clear();
                    // stmts->stats.reserve(oldStats.size());

                    // for (auto &s : oldStats) {
                    //     // std::cout << "s: " << s.showRaw(ctx) << std::endl;
                    //     auto newStat = insertCast(ctx, s);
                    //     stmts->stats.emplace_back(std::move(newStat));
                    // }

                    auto newStat = insertCast(ctx, stmts->expr);
                    stmts->expr = std::move(newStat);
                }

                classDef->rhs.emplace_back(std::move(stat));
                continue;
            }

            if (isAttr(stat)) {
                auto attr = ast::cast_tree<ast::Send>(stat);
                auto comments = findRBSComments(ctx.file.data(ctx).source(), attr->loc);

                for (auto &signature : comments.signatures) {
                    auto docLoc = signature.loc;
                    auto doc = signature.signature;
                    auto rbsType = rbs::RBSParser::parseType(ctx, docLoc, attr->loc, doc);

                    if (rbsType != Qnil) {
                        auto sig =
                            rbs::MethodTypeTranslator::attrSignature(ctx, docLoc, attr, rbsType, comments.annotations);
                        classDef->rhs.emplace_back(std::move(sig));
                    }
                }

                classDef->rhs.emplace_back(std::move(stat));
                continue;

                classDef->rhs.emplace_back(std::move(stat));
                continue;
            }

            auto newStat = insertCast(ctx, stat);
            classDef->rhs.emplace_back(std::move(newStat));
        }
    }

    void preTransformInsSeq(core::MutableContext ctx, ast::ExpressionPtr &tree) {
        auto *insSeq = ast::cast_tree<ast::InsSeq>(tree);
        if (!insSeq) {
            return;
        }

        auto oldStats = std::move(insSeq->stats);
        insSeq->stats.clear();
        insSeq->stats.reserve(oldStats.size());

        for (auto &stat : oldStats) {
            // std::cout << "stat: " << stat.showRaw(ctx) << std::endl;
            auto newStat = insertCast(ctx, stat);
            insSeq->stats.emplace_back(std::move(newStat));
        }

        auto newStat = insertCast(ctx, insSeq->expr);
        insSeq->expr = std::move(newStat);
    }
};

ast::ExpressionPtr RBSSignatures::run(core::MutableContext ctx, ast::ExpressionPtr tree) {
    RBSSignaturesWalk rbs_translate(ctx);
    ast::TreeWalk::apply(ctx, rbs_translate, tree);

    return tree;
}
}; // namespace sorbet::rewriter
