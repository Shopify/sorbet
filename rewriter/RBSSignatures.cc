#include "rewriter/RBSSignatures.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "ast/Helpers.h"
#include "ast/treemap/treemap.h"
#include "core/errors/rewriter.h"
#include "rbs/SignatureTranslator.h"

using namespace std;

namespace sorbet::rewriter {

namespace {

const string_view RBS_PREFIX = "#:";
const string_view ANNOTATION_PREFIX = "# @";

/**
 * Strip ASCII whitespace from the beginning and end of a string_view, but keep track of
 * how many whitespace characters were removed from the beginning.
 */
string_view stripAsciiWhitespaceWithCount(string_view str, uint32_t &prefixWhitespaceCount) {
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

    vector<ast::ExpressionPtr> makeMethodDefSignatures(core::MutableContext ctx, ast::MethodDef *methodDef) {
        auto signatures = vector<ast::ExpressionPtr>();
        auto lineStart = core::Loc::pos2Detail(ctx.file.data(ctx), methodDef->loc.beginLoc).line;
        auto methodName = methodDef->name.toString(ctx);
        auto methodComments = RBSSignatures::getMethodSignatureFor(lineStart);
        auto signatureTranslator = rbs::SignatureTranslator(ctx);

        for (auto &signature : methodComments.signatures) {
            auto sig = signatureTranslator.translateSignature(methodDef, signature, methodComments.annotations);
            if (sig) {
                signatures.emplace_back(move(sig));
            }
        }

        return signatures;
    }

    vector<ast::ExpressionPtr> makeAccessorSignatures(core::MutableContext ctx, ast::Send *send) {
        auto signatures = vector<ast::ExpressionPtr>();
        auto lineStart = core::Loc::pos2Detail(ctx.file.data(ctx), send->loc.beginLoc).line;
        auto attrComments = RBSSignatures::getMethodSignatureFor(lineStart);
        auto signatureTranslator = rbs::SignatureTranslator(ctx);

        for (auto &signature : attrComments.signatures) {
            auto sig = signatureTranslator.translateType(send, signature, attrComments.annotations);
            if (sig) {
                signatures.emplace_back(move(sig));
            }
        }

        return signatures;
    }

public:
    RBSSignaturesWalk(core::MutableContext ctx) {}

    void postTransformClassDef(core::MutableContext ctx, ast::ExpressionPtr &tree) {
        auto &classDef = ast::cast_tree_nonnull<ast::ClassDef>(tree);

        auto newRHS = ast::ClassDef::RHS_store();
        newRHS.reserve(classDef.rhs.size());

        for (auto &stat : classDef.rhs) {
            if (auto methodDef = ast::cast_tree<ast::MethodDef>(stat)) {
                auto signatures = makeMethodDefSignatures(ctx, methodDef);
                for (auto &signature : signatures) {
                    newRHS.emplace_back(move(signature));
                }
            } else if (auto send = ast::cast_tree<ast::Send>(stat)) {
                if (isAccessor(send)) {
                    auto signatures = makeAccessorSignatures(ctx, send);
                    for (auto &signature : signatures) {
                        newRHS.emplace_back(move(signature));
                    }
                } else if (auto methodDef = asVisibilityWrappedMethod(send)) {
                    auto signatures = makeMethodDefSignatures(ctx, methodDef);
                    for (auto &signature : signatures) {
                        newRHS.emplace_back(move(signature));
                    }
                }
            }

            newRHS.emplace_back(move(stat));
        }

        classDef.rhs = move(newRHS);
    }

    void postTransformBlock(core::MutableContext ctx, ast::ExpressionPtr &tree) {
        auto &block = ast::cast_tree_nonnull<ast::Block>(tree);

        if (auto methodDef = ast::cast_tree<ast::MethodDef>(block.body)) {
            auto signatures = makeMethodDefSignatures(ctx, methodDef);

            if (signatures.empty()) {
                return;
            }

            auto newBody = ast::InsSeq::STATS_store();
            newBody.reserve(signatures.size());

            for (auto &signature : signatures) {
                newBody.emplace_back(move(signature));
            }

            block.body = ast::MK::InsSeq(block.loc, move(newBody), move(block.body));

            return;
        }

        if (auto body = ast::cast_tree<ast::InsSeq>(block.body)) {
            auto newBody = ast::InsSeq::STATS_store();

            for (auto &stat : body->stats) {
                if (auto methodDef = ast::cast_tree<ast::MethodDef>(stat)) {
                    auto signatures = makeMethodDefSignatures(ctx, methodDef);
                    for (auto &signature : signatures) {
                        newBody.emplace_back(move(signature));
                    }
                }

                newBody.emplace_back(move(stat));
            }

            if (auto methodDef = ast::cast_tree<ast::MethodDef>(body->expr)) {
                auto signatures = makeMethodDefSignatures(ctx, methodDef);
                for (auto &signature : signatures) {
                    newBody.emplace_back(move(signature));
                }
            }

            body->stats = move(newBody);

            return;
        }
    }
};

} // namespace

// @kaan: TODO might be better to supply this as an argument instead of `thread_local`
thread_local UnorderedMap<std::string, RBSSignatures::Comments> RBSSignatures::methodSignatures;

RBSSignatures::Comments RBSSignatures::getMethodSignatureFor(const uint32_t lineNumber) {
    string key = to_string(lineNumber);

    // fmt::print("Looking for key: {}\n", key);
    // fmt::print("Map contents:\n");
    // for (const auto &[k, v] : methodSignatures) {
    //     fmt::print("  Line {}: {} signatures, {} annotations\n", k, v.signatures.size(), v.annotations.size());
    //     for (const auto &sig : v.signatures) {
    //         fmt::print("    Signature: {}\n", sig.string);
    //     }
    // }
    // fmt::print("\n");

    auto it = methodSignatures.find(key);
    if (it != methodSignatures.end()) {
        Comments result = it->second;
        methodSignatures.erase(it); // Remove the entry from the map to identify unused comments later on
        return result;
    }
    return Comments{};
}

// Triggered once per file
// We iterate over the source code and look for RBS comments
// Each RBS comment is stored in a hash map, methodSignatures. Key is the method name, value is a vector of Comments
void RBSSignatures::extractRBSComments(string_view sourceCode) {
    methodSignatures.clear();

    auto lines = absl::StrSplit(sourceCode, '\n');
    Comments currentComments;
    uint32_t offset = 0;
    uint32_t lineNumber = 0;

    for (const auto &line : lines) {
        lineNumber++;
        uint32_t leadingWhitespaceCount = 0;
        string_view trimmedLine = stripAsciiWhitespaceWithCount(line, leadingWhitespaceCount);

        // Empty lines between the RBS Comment and the method definition are allowed
        if (trimmedLine.empty()) {
            offset += line.length() + 1;
            continue;
        }

        if (absl::StartsWith(trimmedLine, RBS_PREFIX)) {
            auto signature = absl::StripPrefix(trimmedLine, RBS_PREFIX);
            uint32_t startOffset = offset + leadingWhitespaceCount;

            rbs::Comment comment{
                core::LocOffsets{startOffset, static_cast<uint32_t>(startOffset + trimmedLine.length())},
                signature,
            };
            currentComments.signatures.emplace_back(move(comment));
            offset += line.length() + 1;
            continue;
        } else if (absl::StartsWith(trimmedLine, ANNOTATION_PREFIX)) {
            auto annotation = absl::StripPrefix(trimmedLine, ANNOTATION_PREFIX);
            uint32_t startOffset = offset + leadingWhitespaceCount;

            rbs::Comment comment{
                core::LocOffsets{startOffset, static_cast<uint32_t>(startOffset + trimmedLine.length())},
                annotation,
            };
            currentComments.annotations.emplace_back(move(comment));
            offset += line.length() + 1;
            continue;
        } else if (absl::StartsWith(trimmedLine, "#")) {
            offset += line.length() + 1;
            continue;
        }

        if (!currentComments.signatures.empty() || !currentComments.annotations.empty()) {
            string key = to_string(lineNumber);
            methodSignatures[key] = move(currentComments);
        }

        // Clean up currentComments for next iteration
        currentComments = Comments{};
        offset += line.length() + 1;
    }
}

ast::ExpressionPtr RBSSignatures::run(core::MutableContext ctx, ast::ExpressionPtr tree) {
    RBSSignaturesWalk rbsTranslate(ctx);
    ast::TreeWalk::apply(ctx, rbsTranslate, tree);

    // Check for unused comments after processing all methods
    if (!methodSignatures.empty()) {
        for (const auto &[lineNumber, comments] : methodSignatures) {
            // Report each unused signature comment
            for (const auto &sig : comments.signatures) {
                // Use the signature's location offsets directly
                if (auto e = ctx.beginError(sig.loc, core::errors::Rewriter::RBSUnsupported)) {
                    e.setHeader("Unused type annotation. No method def before next annotation");
                    e.addErrorSection(core::ErrorSection(""));
                }
            }
        }
    }

    return tree;
}

}; // namespace sorbet::rewriter
