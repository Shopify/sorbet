#ifndef SORBET_RBS_TYPE_PARAMS_TO_PARSER_NODES_PRISM_H
#define SORBET_RBS_TYPE_PARAMS_TO_PARSER_NODES_PRISM_H

#include "parser/prism/Factory.h"
#include "parser/prism/Parser.h"
#include "rbs/rbs_common.h"
#include <vector>

extern "C" {
#include "prism.h"
}

namespace sorbet::rbs {

class TypeParamsToParserNodesPrism {
    core::MutableContext ctx;
    const Parser &parser;
    parser::Prism::Parser &prismParser;
    const parser::Prism::Factory prism;
    parser::Prism::ParseResult &parseResult;

public:
    TypeParamsToParserNodesPrism(core::MutableContext ctx, const Parser &parser, parser::Prism::Parser &prismParser,
                                 parser::Prism::ParseResult &parseResult)
        : ctx(ctx), parser(parser), prismParser(prismParser), prism{prismParser}, parseResult(parseResult) {}

    std::vector<pm_node_t *> typeParams(const rbs_node_list_t *rbsTypeParams, const RBSDeclaration &declaration);
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_TYPE_PARAMS_TO_PARSER_NODES_PRISM_H
