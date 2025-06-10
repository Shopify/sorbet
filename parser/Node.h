#ifndef SORBET_PARSER_NODE_H
#define SORBET_PARSER_NODE_H

#include "ast/Trees.h"
#include "common/common.h"
#include "core/core.h"
#include <memory>
#include <string>

namespace sorbet::parser {

class Node {
public:
    Node(core::LocOffsets loc) : loc(loc) {
        ENFORCE(loc.exists(), "Location of parser node is none");
    }

    Node(core::LocOffsets loc, bool enforceLocationExists) : loc(loc) {
        if (enforceLocationExists) {
            ENFORCE(loc.exists(), "Location of parser node is none");
        }
    }
    virtual ~Node() = default;
    virtual std::string toStringWithTabs(const core::GlobalState &gs, int tabs = 0) const = 0;
    std::string toString(const core::GlobalState &gs) const {
        return toStringWithTabs(gs);
    }
    virtual std::string toJSON(const core::GlobalState &gs, int tabs = 0) = 0;
    virtual std::string toJSONWithLocs(const core::GlobalState &gs, core::FileRef file, int tabs = 0) = 0;
    virtual std::string toWhitequark(const core::GlobalState &gs, int tabs = 0) = 0;
    virtual std::string nodeName() = 0;

    virtual ast::ExpressionPtr getCachedDesugaredExpr() {
        return nullptr;
    }

    virtual void cacheDesugaredExpr(ast::ExpressionPtr expr) {
        // no-op, except for the override in NodeAndExpr
    }

    core::LocOffsets loc;

protected:
    void printTabs(fmt::memory_buffer &to, int count) const;
    void printNode(fmt::memory_buffer &to, const std::unique_ptr<Node> &node, const core::GlobalState &gs,
                   int tabs) const;
    void printNodeJSON(fmt::memory_buffer &to, const std::unique_ptr<Node> &node, const core::GlobalState &gs,
                       int tabs) const;
    void printNodeJSONWithLocs(fmt::memory_buffer &to, const std::unique_ptr<Node> &node, const core::GlobalState &gs,
                               core::FileRef file, int tabs) const;
    void printNodeWhitequark(fmt::memory_buffer &to, const std::unique_ptr<Node> &node, const core::GlobalState &gs,
                             int tabs) const;
};

class ExprWrapperNode : public parser::Node {
    ast::ExpressionPtr desugaredExpr = nullptr;
public:
    ExprWrapperNode(core::LocOffsets loc, ast::ExpressionPtr expr) : Node(loc, false), desugaredExpr(std::move(expr)) {
        categoryCounterInc("nodes", "ExprWrapperNode");
    }

    virtual std::string toStringWithTabs(const core::GlobalState &gs,
                                       int tabs = 0)  const {
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), "ExprWrapperNode {{\n");
        printTabs(buf, tabs);
        fmt::format_to(std::back_inserter(buf), "}}");
        return to_string(buf);
    }
    virtual std::string toJSON(const core::GlobalState &gs, int tabs = 0) {
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), "{{\n");
        printTabs(buf, tabs + 1);
        fmt::format_to(std::back_inserter(buf), "\"type\" : \"ExprWrapperNode\"\n");
        printTabs(buf, tabs);
        fmt::format_to(std::back_inserter(buf), "}}");
        return to_string(buf);
    }

    virtual std::string toJSONWithLocs(const core::GlobalState &gs,
                                       core::FileRef file, int tabs = 0) {
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), "{{\n");
        printTabs(buf, tabs + 1);
        fmt::format_to(std::back_inserter(buf), "\"type\" : \"ExprWrapperNode\"\n");
        printTabs(buf, tabs);
        fmt::format_to(std::back_inserter(buf), "}}");
        return to_string(buf);
    }

    virtual std::string toWhitequark(const core::GlobalState &gs, int tabs = 0) {
        fmt::memory_buffer buf;
        fmt::format_to(std::back_inserter(buf), "s(:ExprWrapperNode");
        fmt::format_to(std::back_inserter(buf), ")");
        return to_string(buf);
    }

    virtual std::string nodeName() { return "ExprWrapperNode"; };

    virtual ast::ExpressionPtr getCachedDesugaredExpr() {
        // We know each `NodeAndExpr` object's `getCachedDesugaredExpr()` will be called at most once, either:
        // 1. When its parent node is being translated below, and this value is used to create that parent's expr.
        // 2. When this node is visted by `node2TreeImpl` in `Runner.cc`, and this value is used in the fast-path.
        //
        // Because of this, we don't need to make any copies here. Just move this value out,
        // and exclusive ownership to the caller.
        return std::move(this->desugaredExpr);
    }
};

template <class To> To *cast_node(Node *what) {
    static_assert(!std::is_pointer<To>::value, "To has to be a pointer");
    static_assert(std::is_assignable<Node *&, To *>::value, "Ill Formed To, has to be a subclass of Expression");
#if __cplusplus >= 201402L
    // static_assert(std::is_final<To>::value, "To is not final");
#elif __has_feature(is_final)
    static_assert(__is_final(To), "To is not final");
#else
    static_assert(false);
#endif
    return fast_cast<Node, To>(what);
}

template <class To> bool isa_node(Node *what) {
    return cast_node<To>(what) != nullptr;
}

using NodeVec = InlinedVector<std::unique_ptr<Node>, 4>;

#include "parser/Node_gen.h"
}; // namespace sorbet::parser

#endif
