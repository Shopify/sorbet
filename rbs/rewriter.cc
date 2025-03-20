#include "rbs/rewriter.h"

#include "common/typecase.h"
#include "parser/Builder.h"
#include "parser/parser/include/ruby_parser/builder.hh"
#include "parser/parser/include/ruby_parser/driver.hh"
#include "parser/parser/include/ruby_parser/token.hh"

using namespace std;

namespace sorbet::rbs {

unique_ptr<parser::Node> rewriteNode(core::MutableContext ctx, unique_ptr<parser::Node> node) {
    std::cerr << "RBSrewrite" << std::endl;

    if (node == nullptr) {
        return node;
    }

    unique_ptr<parser::Node> result;

    typecase(
        node.get(),
        [&](parser::Begin *begin) {
            std::cerr << "Rewrite begin" << std::endl;
            std::vector<unique_ptr<parser::Node>> oldStmts;
            for (auto &stmt : begin->stmts) {
                oldStmts.push_back(std::move(stmt));
            }

            begin->stmts.clear();
            for (auto &stmt : oldStmts) {
                begin->stmts.push_back(rewriteNode(ctx, std::move(stmt)));
            }
            result = std::move(node);
        },
        [&](parser::Assign *asgn) {
            std::cerr << "Rewrite assign" << std::endl;
            asgn->rhs = rewriteNode(ctx, std::move(asgn->rhs));
            asgn->lhs = rewriteNode(ctx, std::move(asgn->lhs));
            result = std::move(node);
        },
        [&](parser::Integer *integer) {
            std::cerr << "Rewrite integer" << std::endl;

            auto t = (ruby_parser::token *)integer;
            auto loc = core::LocOffsets(t->start(), t->end());
            result = make_unique<parser::Integer>(loc, "42");
        },
        [&](parser::Node *other) { result = std::move(node); });

    return result;
}

} // namespace sorbet::rbs
