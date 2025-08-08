#include "rbs/prism/TypeParamsToParserNodesPrism.h"

#include "core/errors/rewriter.h"
#include "parser/prism/Factory.h"
#include "parser/prism/Parser.h"
#include "rbs/prism/TypeToParserNodePrism.h"
#include "rbs/rbs_common.h"

using namespace std;

namespace sorbet::rbs {

vector<pm_node_t *> TypeParamsToParserNodePrism::typeParams(const rbs_node_list_t *rbsTypeParams,
                                                            const RBSDeclaration &declaration) {
    vector<pm_node_t *> result;
    result.reserve(rbsTypeParams->length);

    for (rbs_node_list_node_t *list_node = rbsTypeParams->head; list_node != nullptr; list_node = list_node->next) {
        ENFORCE(list_node->node->type == RBS_AST_TYPE_PARAM,
                "Unexpected node type `{}` in type parameter list, expected `{}`", rbs_node_type_name(list_node->node),
                "TypeParam");

        auto rbsTypeParam = (rbs_ast_type_param_t *)list_node->node;
        auto loc = declaration.typeLocFromRange(list_node->node->location->rg);

        if (rbsTypeParam->unchecked) {
            if (auto e = ctx.beginIndexerError(loc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("`{}` type parameters are not supported by Sorbet", "unchecked");
            }
        }

        auto nameStr = parser.resolveConstant(rbsTypeParam->name);
        auto nameConstant = ctx.state.enterNameConstant(nameStr);

        vector<pm_node_t *> args;
        if (rbsTypeParam->variance) {
            auto variance = parser.resolveKeyword(rbsTypeParam->variance);
            if (variance == "covariant") {
                args.push_back(prism.Symbol(loc, core::Names::covariant().show(ctx.state)));
            } else if (variance == "contravariant") {
                args.push_back(prism.Symbol(loc, core::Names::contravariant().show(ctx.state)));
            }
        }

        auto defaultType = rbsTypeParam->default_type;
        auto upperBound = rbsTypeParam->upper_bound;
        auto lowerBound = rbsTypeParam->lower_bound;

        pm_node_t *block = nullptr;
        if (defaultType || upperBound || lowerBound) {
            auto typeTranslator =
                TypeToParserNodePrism(ctx, absl::Span<pair<core::LocOffsets, core::NameRef>>{}, parser, prismParser);
            vector<pm_node_t *> pairs;

            if (defaultType) {
                auto key = prism.Symbol(loc, core::Names::fixed().show(ctx.state));
                auto value = typeTranslator.toPrismNode(defaultType, declaration);
                pairs.push_back(prism.AssocNode(loc, key, value));
            }

            if (upperBound) {
                auto key = prism.Symbol(loc, core::Names::upper().show(ctx.state));
                auto value = typeTranslator.toPrismNode(upperBound, declaration);
                pairs.push_back(prism.AssocNode(loc, key, value));
            }

            if (lowerBound) {
                auto key = prism.Symbol(loc, core::Names::lower().show(ctx.state));
                auto value = typeTranslator.toPrismNode(lowerBound, declaration);
                pairs.push_back(prism.AssocNode(loc, key, value));
            }

            auto body = prism.Hash(loc, absl::MakeSpan(pairs));
            block = prism.Block(loc, body);
        }

        auto typeCall = prism.Call(loc, prism.SorbetPrivateStatic(loc), "type_member"sv, absl::MakeSpan(args), block);

        auto assign = prism.ConstantWriteNode(loc, prism.addConstantToPool(nameConstant.show(ctx.state)), typeCall);
        result.push_back(assign);
    }

    return result;
}

} // namespace sorbet::rbs
