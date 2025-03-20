#include "rbs/rewriter.h"

#include "common/typecase.h"
#include "parser/Builder.h"
#include "parser/parser.h"
#include "parser/parser/include/ruby_parser/builder.hh"
#include "parser/parser/include/ruby_parser/driver.hh"
#include "parser/parser/include/ruby_parser/token.hh"

using namespace std;

namespace sorbet::rbs {

namespace {

// core::LocOffsets tokLoc(const ruby_parser::token *tok) {
//     return core::LocOffsets(tok->start(), tok->end());
// }

}; // namespace

unique_ptr<parser::Node> rewriteNode(core::MutableContext ctx, unique_ptr<parser::Node> node) {
    if (ctx.file.data(ctx.state).path() != "test.rb") {
        return node;
    }

    std::cerr << "RBSrewrite" << std::endl;

    if (node == nullptr) {
        return node;
    }

    unique_ptr<parser::Node> result;

    typecase(
        node.get(),
        // Nodes are ordered as in desugar
        [&](parser::Const *const_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Send *send) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::String *string) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Symbol *symbol) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::LVar *var) { result = std::move(node); },
        [&](parser::Hash *hash) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Block *block) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Begin *begin) {
            std::cerr << "Rewrite begin" << std::endl;
            std::vector<unique_ptr<parser::Node>> oldStmts;
            for (auto &stmt : begin->stmts) {
                oldStmts.push_back(std::move(stmt));
            }

            begin->stmts.clear();
            for (auto &stmt : oldStmts) {
                begin->stmts.push_back(rewriteNode(ctx, move(stmt)));
            }
            result = move(node);
        },
        [&](parser::Assign *asgn) {
            std::cerr << "Rewrite assign" << std::endl;
            asgn->lhs = rewriteNode(ctx, move(asgn->lhs));

            auto cbase = make_unique<parser::Cbase>(asgn->rhs->loc);
            auto recv = make_unique<parser::Const>(asgn->rhs->loc, move(cbase), core::Names::Constants::T());
            auto method = core::Names::cast();

            auto args = parser::NodeVec();
            auto value = rewriteNode(ctx, move(asgn->rhs));
            args.push_back(move(value));
            auto type = make_unique<parser::Const>(asgn->lhs->loc, nullptr, core::Names::Constants::String());
            args.push_back(move(type));

            auto send = make_unique<parser::Send>(asgn->lhs->loc, move(recv), method, asgn->lhs->loc, move(args));

            asgn->rhs = move(send);

            result = move(node);
        },
        // END hand-ordered clauses
        //[&](parser::And *and_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Or *or_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::AndAsgn *andAsgn) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::OrAsgn *orAsgn) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::OpAsgn *opAsgn) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::CSend *csend) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Self *self) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::DSymbol *dsymbol) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::FileLiteral *fileLiteral) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName());
        //},
        //[&](parser::ConstLhs *constLhs) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Cbase *cbase) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Kwbegin *kwbegin) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Module *module) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Class *klass) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Arg *arg) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Restarg *arg) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Kwrestarg *arg) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Kwarg *arg) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Blockarg *arg) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Kwoptarg *arg) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Optarg *arg) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Shadowarg *arg) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::DefMethod *method) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::DefS *method) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::SClass *sclass) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::NumBlock *block) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::While *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::WhilePost *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Until *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::UntilPost *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Nil *wl) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::IVar *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::GVar *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::CVar *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::LVarLhs *var) { result = move(node); },
        //[&](parser::GVarLhs *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::CVarLhs *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::IVarLhs *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::NthRef *var) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Super *super) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::ZSuper *zuper) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::For *for_) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        [&](parser::Integer *integer) { result = move(node); },
        //[&](parser::DString *dstring) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Float *floatNode) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Complex *complex) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Rational *complex) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
        //[&](parser::Array *array) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
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
        //[&](parser::Masgn *masgn) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); },
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
        [&](parser::Node *other) { Exception::raise("Unimplemented Parser Node: {}", node->nodeName()); });

    return result;
}

} // namespace sorbet::rbs
