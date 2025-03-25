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

// TODO: @kaan method name logic, vibed too hard
const string_view DEF_PREFIX = "def ";
const string_view PUBLIC_DEF_PREFIX = "public def ";
const string_view PROTECTED_DEF_PREFIX = "protected def ";
const string_view PRIVATE_DEF_PREFIX = "private def ";
const string_view PRIVATE_CLASS_METHOD_DEF_PREFIX = "private_class_method def ";
const string_view PUBLIC_CLASS_METHOD_DEF_PREFIX = "public_class_method def ";
const string_view PACKAGE_PRIVATE_DEF_PREFIX = "package_private def ";
const string_view PACKAGE_PRIVATE_CLASS_METHOD_DEF_PREFIX = "package_private_class_method def ";
const string_view RBS_PREFIX = "#:";
const string_view ANNOTATION_PREFIX = "# @";

const string_view ATTR_READER_PREFIX = "attr_reader ";
const string_view ATTR_WRITER_PREFIX = "attr_writer ";
const string_view ATTR_ACCESSOR_PREFIX = "attr_accessor ";

string_view extractMethodName(string_view line, string_view prefix) {
    auto nameStart = line.substr(prefix.length());
    size_t methodEnd = nameStart.find_first_of(" \t\r\n(;");

    // If no terminator found, use the entire remaining string
    if (methodEnd == string_view::npos) {
        return absl::StripAsciiWhitespace(nameStart);
    }
    return absl::StripAsciiWhitespace(nameStart.substr(0, methodEnd));
}

// Create a struct to hold both the method name and its actual offset
struct MethodInfo {
    string_view name;
    uint32_t defOffset;
};

MethodInfo findMethodName(string_view line, uint32_t baseOffset) {
    if (absl::StartsWith(line, DEF_PREFIX)) {
        return {extractMethodName(line, DEF_PREFIX), 0};
    } else if (absl::StartsWith(line, PUBLIC_DEF_PREFIX)) {
        return {extractMethodName(line, PUBLIC_DEF_PREFIX),
                static_cast<uint32_t>(PUBLIC_DEF_PREFIX.length() - DEF_PREFIX.length())};
    } else if (absl::StartsWith(line, PROTECTED_DEF_PREFIX)) {
        return {extractMethodName(line, PROTECTED_DEF_PREFIX),
                static_cast<uint32_t>(PROTECTED_DEF_PREFIX.length() - DEF_PREFIX.length())};
    } else if (absl::StartsWith(line, PRIVATE_DEF_PREFIX)) {
        return {extractMethodName(line, PRIVATE_DEF_PREFIX),
                static_cast<uint32_t>(PRIVATE_DEF_PREFIX.length() - DEF_PREFIX.length())};
    } else if (absl::StartsWith(line, PRIVATE_CLASS_METHOD_DEF_PREFIX)) {
        return {extractMethodName(line, PRIVATE_CLASS_METHOD_DEF_PREFIX),
                static_cast<uint32_t>(PRIVATE_CLASS_METHOD_DEF_PREFIX.length() - DEF_PREFIX.length())};
    } else if (absl::StartsWith(line, PUBLIC_CLASS_METHOD_DEF_PREFIX)) {
        return {extractMethodName(line, PUBLIC_CLASS_METHOD_DEF_PREFIX),
                static_cast<uint32_t>(PUBLIC_CLASS_METHOD_DEF_PREFIX.length() - DEF_PREFIX.length())};
    } else if (absl::StartsWith(line, PACKAGE_PRIVATE_DEF_PREFIX)) {
        return {extractMethodName(line, PACKAGE_PRIVATE_DEF_PREFIX),
                static_cast<uint32_t>(PACKAGE_PRIVATE_DEF_PREFIX.length() - DEF_PREFIX.length())};
    } else if (absl::StartsWith(line, PACKAGE_PRIVATE_CLASS_METHOD_DEF_PREFIX)) {
        return {extractMethodName(line, PACKAGE_PRIVATE_CLASS_METHOD_DEF_PREFIX),
                static_cast<uint32_t>(PACKAGE_PRIVATE_CLASS_METHOD_DEF_PREFIX.length() - DEF_PREFIX.length())};
    } else if (absl::StartsWith(line, ATTR_READER_PREFIX)) {
        return {extractMethodName(line, ATTR_READER_PREFIX), 0};
    } else if (absl::StartsWith(line, ATTR_WRITER_PREFIX)) {
        return {extractMethodName(line, ATTR_WRITER_PREFIX), 0};
    } else if (absl::StartsWith(line, ATTR_ACCESSOR_PREFIX)) {
        return {extractMethodName(line, ATTR_ACCESSOR_PREFIX), 0};
    }
    return {string_view(), 0};
}

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
        auto methodComments = RBSSignatures::getMethodSignatureFor(methodDef->name.toString(ctx), methodDef->loc);
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
        if (!send->hasPosArgs()) {
            return signatures;
        }
        auto attrComments = RBSSignatures::getMethodSignatureFor(send->getPosArg(0).toString(ctx), send->loc);
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

thread_local UnorderedMap<std::string, RBSSignatures::Comments> RBSSignatures::methodSignatures;

RBSSignatures::Comments RBSSignatures::getMethodSignatureFor(const string_view &methodName,
                                                             const core::LocOffsets &loc) {
    string key = to_string(loc.beginPos()) + string(methodName);
    auto &map = methodSignatures; // Get a local reference
    auto it = map.find(key);
    if (it != map.end()) {
        return it->second;
    }
    // Print the contents of the methodSignatures map for debugging
    fmt::print("Method signatures map contents:\n");
    for (const auto &[key, comments] : map) {
        fmt::print("  Key: {}\n", key);

        // Print RBS signatures
        fmt::print("    Signatures ({}):\n", comments.signatures.size());
        for (const auto &sig : comments.signatures) {
            fmt::print("      Loc: {}:{}, Content: {}\n", sig.loc.beginPos(), sig.loc.endPos(), sig.string);
        }

        // Print annotations
        fmt::print("    Annotations ({}):\n", comments.annotations.size());
        for (const auto &anno : comments.annotations) {
            fmt::print("      Loc: {}:{}, Content: {}\n", anno.loc.beginPos(), anno.loc.endPos(), anno.string);
        }
    }
    fmt::print("key: {}\n", key);

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

    for (const auto &line : lines) {
        uint32_t leadingWhitespaceCount = 0;
        string_view trimmedLine = stripAsciiWhitespaceWithCount(line, leadingWhitespaceCount);

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
            // Skip regular comments
            offset += line.length() + 1;
            continue;
        }

        // @kaan: TODO sig block between method and RBS comment, already logic for braces and do block

        // @kaan: We may be broken for class definitions, or remaining comments

        if (!currentComments.signatures.empty() || !currentComments.annotations.empty()) {
            uint32_t lineOffset = offset + (trimmedLine.data() - line.data());
            MethodInfo info = findMethodName(trimmedLine, lineOffset);
            string_view methodName = info.name;
            uint32_t defAdjustment = info.defOffset;

            if (!methodName.empty()) {
                uint32_t actualDefOffset = lineOffset + defAdjustment;
                string key = to_string(actualDefOffset) + string(methodName);
                // @kaan: Do we need to store the method name or offset is enough?
                methodSignatures[key] = move(currentComments);
            }
            currentComments = Comments{};
        }
        offset += line.length() + 1;
    }
}

ast::ExpressionPtr RBSSignatures::run(core::MutableContext ctx, ast::ExpressionPtr tree) {
    RBSSignaturesWalk rbsTranslate(ctx);
    ast::TreeWalk::apply(ctx, rbsTranslate, tree);

    return tree;
}

}; // namespace sorbet::rewriter
