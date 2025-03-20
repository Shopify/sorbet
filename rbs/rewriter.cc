#include "rbs/rewriter.h"

#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "common/typecase.h"
#include "core/errors/rewriter.h"
#include "parser/Builder.h"
#include "parser/helper.h"
#include "parser/parser.h"
#include "parser/parser/include/ruby_parser/builder.hh"
#include "parser/parser/include/ruby_parser/driver.hh"
#include "parser/parser/include/ruby_parser/token.hh"
#include "rbs/RBSParser.h"
#include "rbs/TypeTranslator.h"
#include "rbs/rbs_common.h"

using namespace std;

namespace sorbet::rbs {

namespace {

// core::LocOffsets tokLoc(const ruby_parser::token *tok) {
//     return core::LocOffsets(tok->start(), tok->end());
// }

optional<rbs::Comment> findRBSComments(core::MutableContext ctx, unique_ptr<parser::Node> &node) {
    auto source = ctx.file.data(ctx).source();

    // We want to find the comment right after the end of the assign
    auto startingLoc = node->loc.endPos();

    // Get the position of the end of the line from the startingLoc
    auto endOfLine = source.find('\n', startingLoc);
    if (endOfLine == string::npos) {
        return nullopt;
    }

    // Check between the startingLoc and the end of the line for a `#: ...` comment
    auto comment = source.substr(startingLoc, endOfLine - startingLoc);

    // Find the position of the `#:` in the comment
    auto commentStart = comment.find("#:");
    if (commentStart == string::npos) {
        return nullopt;
    }

    // Adjust the location to be the correct position depending on the number of spaces after the `#:`
    auto offset = 0;
    for (auto i = startingLoc + commentStart + 2; i < endOfLine; i++) {
        if (source[i] == ' ') {
            offset++;
        } else {
            break;
        }
    }

    return rbs::Comment{
        core::LocOffsets{startingLoc + (uint32_t)commentStart + offset, static_cast<uint32_t>(endOfLine)},
        absl::StripAsciiWhitespace(comment.substr(commentStart + 2))};
}

/**
 * Get the RBS type from the given assign
 */
unique_ptr<parser::Node> getRBSAssertionType(core::MutableContext ctx, unique_ptr<parser::Node> &node) {
    auto assertion = findRBSComments(ctx, node);

    if (!assertion) {
        return nullptr;
    }

    auto result = rbs::RBSParser::parseType(ctx, *assertion);
    if (result.second) {
        if (auto e = ctx.beginError(result.second->loc, core::errors::Rewriter::RBSSyntaxError)) {
            e.setHeader("Failed to parse RBS type ({})", result.second->message);
        }
        return nullptr;
    }

    auto rbsType = move(result.first.value());
    auto typeParams = vector<pair<core::LocOffsets, core::NameRef>>();
    return rbs::TypeTranslator::toParserNode(ctx, typeParams, rbsType.node.get(), assertion->loc);
}

parser::NodeVec rewriteNodes(core::MutableContext ctx, parser::NodeVec nodes) {
    for (auto &node : nodes) {
        node = rewriteNode(ctx, move(node));
    }

    return nodes;
}

}; // namespace

unique_ptr<parser::Node> rewriteNode(core::MutableContext ctx, unique_ptr<parser::Node> node) {
    if (node == nullptr) {
        return node;
    }

    unique_ptr<parser::Node> result;

    typecase(
        node.get(),
        // Nodes are ordered as in desugar
        [&](parser::Const *const_) { result = move(node); },
        [&](parser::Send *send) {
            send->receiver = rewriteNode(ctx, move(send->receiver));
            send->args = rewriteNodes(ctx, move(send->args));
            result = move(node);
        },
        [&](parser::String *string) { result = move(node); }, [&](parser::Symbol *symbol) { result = move(node); },
        [&](parser::LVar *var) { result = std::move(node); }, [&](parser::Hash *hash) { result = move(node); },
        [&](parser::Block *block) {
            block->body = rewriteNode(ctx, move(block->body));
            result = move(node);
        },
        [&](parser::Begin *begin) {
            begin->stmts = rewriteNodes(ctx, move(begin->stmts));
            result = move(node);
        },
        [&](parser::Assign *asgn) {
            if (auto rbsType = getRBSAssertionType(ctx, node)) {
                auto rhs = rewriteNode(ctx, move(asgn->rhs));
                asgn->rhs = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        // END hand-ordered clauses
        [&](parser::And *and_) {
            and_->left = rewriteNode(ctx, move(and_->left));
            and_->right = rewriteNode(ctx, move(and_->right));
            result = move(node);
        },
        [&](parser::Or *or_) {
            or_->left = rewriteNode(ctx, move(or_->left));
            or_->right = rewriteNode(ctx, move(or_->right));
            result = move(node);
        },
        [&](parser::AndAsgn *andAsgn) {
            if (auto rbsType = getRBSAssertionType(ctx, node)) {
                auto rhs = rewriteNode(ctx, move(andAsgn->right));
                andAsgn->right = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        [&](parser::OrAsgn *orAsgn) {
            if (auto rbsType = getRBSAssertionType(ctx, node)) {
                auto rhs = rewriteNode(ctx, move(orAsgn->right));
                orAsgn->right = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        [&](parser::OpAsgn *opAsgn) {
            if (auto rbsType = getRBSAssertionType(ctx, node)) {
                auto rhs = rewriteNode(ctx, move(opAsgn->right));
                opAsgn->right = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        [&](parser::CSend *csend) {
            csend->receiver = rewriteNode(ctx, move(csend->receiver));
            csend->args = rewriteNodes(ctx, move(csend->args));
            result = move(node);
        },
        //[&](parser::Self *self) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::DSymbol *dsymbol) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::FileLiteral *fileLiteral) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
        //},
        [&](parser::ConstLhs *constLhs) { result = move(node); },
        //[&](parser::Cbase *cbase) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Kwbegin *kwbegin) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Module *module) {
            module->body = rewriteNode(ctx, move(module->body));
            result = move(node);
        },
        [&](parser::Class *klass) {
            klass->body = rewriteNode(ctx, move(klass->body));
            result = move(node);
        },
        // [&](parser::Args *args) {
        //     args->args = rewriteNodes(ctx, move(args->args));
        //     result = move(node);
        // },
        // [&](parser::Arg *arg) { result = move(node); }, [&](parser::Restarg *arg) { result = move(node); },
        // [&](parser::Kwrestarg *arg) { result = move(node); }, [&](parser::Kwarg *arg) { result = move(node); },
        // [&](parser::Blockarg *arg) { result = move(node); }, [&](parser::Kwoptarg *arg) { result = move(node); },
        // [&](parser::Optarg *arg) { result = move(node); }, [&](parser::Shadowarg *arg) { result = move(node); },
        [&](parser::DefMethod *method) {
            // method->args = rewriteNode(ctx, move(method->args));
            method->body = rewriteNode(ctx, move(method->body));
            result = move(node);
        },
        [&](parser::DefS *method) {
            // method->args = rewriteNode(ctx, move(method->args));
            method->body = rewriteNode(ctx, move(method->body));
            result = move(node);
        },
        [&](parser::SClass *sclass) {
            sclass->body = rewriteNode(ctx, move(sclass->body));
            result = move(node);
        },
        //[&](parser::NumBlock *block) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::While *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::WhilePost *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Until *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::UntilPost *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Nil *wl) { result = move(node); }, [&](parser::IVar *var) { result = move(node); },
        [&](parser::GVar *var) { result = move(node); }, [&](parser::CVar *var) { result = move(node); },
        [&](parser::LVarLhs *var) { result = move(node); },
        //[&](parser::GVarLhs *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::CVarLhs *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::IVarLhs *var) { result = move(node); },
        //[&](parser::NthRef *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Super *super) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::ZSuper *zuper) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::For *for_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Integer *integer) { result = move(node); },
        //[&](parser::DString *dstring) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Float *floatNode) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Complex *complex) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Rational *complex) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Array *array) { result = move(node); },
        //[&](parser::IRange *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::ERange *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Regexp *regexpNode) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Regopt *regopt) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Return *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Break *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Next *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Retry *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Yield *ret) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Rescue *rescue) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Resbody *resbody) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Ensure *ensure) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::If *if_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Masgn *masgn) {
            if (auto rbsType = getRBSAssertionType(ctx, node)) {
                auto rhs = rewriteNode(ctx, move(masgn->rhs));
                masgn->rhs = parser::MK::TLet(rbsType->loc, move(rhs), move(rbsType));
            }

            result = move(node);
        },
        //[&](parser::True *t) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::False *t) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Case *case_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Splat *splat) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::ForwardedRestArg *fra) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Alias *alias) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Defined *defined) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::LineLiteral *line) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::XString *xstring) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Preexe *preexe) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Postexe *postexe) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Undef *undef) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::CaseMatch *caseMatch) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Backref *backref) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::EFlipflop *eflipflop) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::IFlipflop *iflipflop) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::MatchCurLine *matchCurLine) {
        //    Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
        //},
        //[&](parser::Redo *redo) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::EncodingLiteral *encodingLiteral) {
        //    Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
        //},
        //[&](parser::MatchPattern *pattern) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::MatchPatternP *pattern) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::EmptyElse *else_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::BlockPass *blockPass) { Exception::raise("Send should have already handled the BlockPass"); },
        [&](parser::Node *other) {
            std::cerr << "Unimplemented Parser Node: " << node->nodeName() << std::endl;
            Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
            exit(1);
        });

    return result;
}

} // namespace sorbet::rbs
