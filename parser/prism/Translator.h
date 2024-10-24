#ifndef SORBET_PARSER_PRISM_TRANSLATOR_H
#define SORBET_PARSER_PRISM_TRANSLATOR_H

#include <memory>

#include "../Node.h" // To clarify: these are Sorbet Parser nodes, not Prism ones.
#include "parser/prism/Parser.h"

extern "C" {
#include "prism.h"
}

namespace sorbet::parser::Prism {

class Translator final {
    Parser parser;

    // The functions in Pipeline.cc pass around a reference to the global state as a parameter,
    // but don't have explicit ownership over it. We take a temporary reference to it, but we can't
    // escape that scope, which is why Translator objects can't be copied, or even moved.
    core::GlobalState &gs;
    uint16_t uniqueCounter = 1;

    Translator(Translator &&) = delete;                 // Move constructor
    Translator(const Translator &) = delete;            // Copy constructor
    Translator &operator=(Translator &&) = delete;      // Move assignment
    Translator &operator=(const Translator &) = delete; // Copy assignment
public:
    Translator(Parser parser, core::GlobalState &gs) : parser(parser), gs(gs) {} // Default constructor

    // Translates the given AST from Prism's node types into the equivalent AST in Sorbet's legacy parser node types.
    std::unique_ptr<parser::Node> translate(pm_node_t *node);
    std::unique_ptr<parser::Node> translate(const Node &node);

private:
    core::LocOffsets translateLoc(pm_location_t loc);

    parser::NodeVec translateMulti(pm_node_list prismNodes);
    void translateMultiInto(NodeVec &sorbetNodes, absl::Span<pm_node_t *> prismNodes);

    NodeVec translateArguments(pm_arguments_node *node, size_t extraCapacity = 0);
    std::unique_ptr<parser::Hash> translateHash(pm_node_t *node, pm_node_list_t elements,
                                                bool isUsedForKeywordArguments);
    std::unique_ptr<parser::Node> translateCallWithBlock(pm_node_t *prismBlockOrLambdaNode,
                                                         std::unique_ptr<parser::Node> sendNode);
    std::unique_ptr<parser::Node> translateRescue(pm_rescue_node *prismRescueNode,
                                                  std::unique_ptr<parser::Node> beginNode,
                                                  std::unique_ptr<parser::Node> elseNode);
    std::unique_ptr<parser::Node> translateStatements(pm_statements_node *stmtsNode, bool inlineIfSingle);

    template <typename SorbetNode> std::unique_ptr<SorbetNode> translateSimpleKeyword(pm_node_t *untypedNode);

    std::unique_ptr<parser::Regopt> translateRegexpOptions(pm_location_t closingLoc);
    std::unique_ptr<parser::Regexp> translateRegexp(pm_string_t unescaped, core::LocOffsets location,
                                                    pm_location_t closingLoc);

    template <typename PrismAssignmentNode, typename SorbetLHSNode>
    std::unique_ptr<parser::Assign> translateAssignment(pm_node_t *node);

    template <typename PrismAssignmentNode, typename SorbetAssignmentNode, typename SorbetLHSNode>
    std::unique_ptr<SorbetAssignmentNode> translateOpAssignment(pm_node_t *node);

    template <typename PrismLhsNode, typename SorbetLHSNode>
    std::unique_ptr<SorbetLHSNode> translateConst(PrismLhsNode *node);

    // Pattern-matching
    // ... variations of the main translation functions for pattern-matching related nodes.
    std::unique_ptr<parser::Node> patternTranslate(pm_node_t *node);
    parser::NodeVec patternTranslateMulti(pm_node_list prismNodes);
    void patternTranslateMultiInto(NodeVec &sorbetNodes, absl::Span<pm_node_t *> prismNodes);
};

} // namespace sorbet::parser::Prism
#endif // SORBET_PARSER_PRISM_TRANSLATOR_H
