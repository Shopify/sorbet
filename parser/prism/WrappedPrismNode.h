#ifndef SORBET_PARSER_PRISM_PARSER_H
#define SORBET_PARSER_PRISM_PARSER_H

#include "../Node.h" // To clarify: these are Sorbet Parser nodes, not Prism ones.
#include "Parser.h"
#include <memory>
#include <string>

extern "C" {
#include "prism.h"
}

#include "core/LocOffsets.h"

namespace sorbet::parser {

// using PrismNode = ::sorbet::parser::Prism::Node;
using WhitequarkNode = ::sorbet::parser::Node;

// A Whitequark node which wraps a Prism node, for use during our migration to Prism.
class WrappedPrismNode : public WhitequarkNode {
public:
    WrappedPrismNode(Prism::Node node) : WhitequarkNode(node.getLoc()) {}

    void f(Prism::ParserStorage storage) {}

    virtual std::string toStringWithTabs(const core::GlobalState &gs, int tabs = 0) const {
        Exception::raise("WrappedPrismNode::toStringWithTabs not implemented");
    }
    std::string toString(const core::GlobalState &gs) const {
        Exception::raise("WrappedPrismNode::toStringWithTabs not implemented");
    }
    virtual std::string toJSON(const core::GlobalState &gs, int tabs = 0) {
        Exception::raise("WrappedPrismNode::toJSON not implemented");
    }
    virtual std::string toJSONWithLocs(const core::GlobalState &gs, core::FileRef file, int tabs = 0) {
        Exception::raise("WrappedPrismNode::toJSONWithLocs not implemented");
    }
    virtual std::string toWhitequark(const core::GlobalState &gs, int tabs = 0) {
        Exception::raise("WrappedPrismNode::toWhitequark not implemented");
    }
    virtual std::string nodeName() {
        Exception::raise("WrappedPrismNode::nodeName not implemented");
    }
};

} // namespace sorbet::parser
#endif // SORBET_PARSER_PRISM_PARSER_H
