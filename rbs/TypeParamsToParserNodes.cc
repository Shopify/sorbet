#include "rbs/TypeParamsToParserNodes.h"

#include "core/errors/rewriter.h"
#include "parser/helper.h"
#include "parser/parser.h"
#include "rbs/TypeToParserNode.h"
#include "rbs/rbs_common.h"

using namespace std;

namespace sorbet::rbs {

parser::NodeVec TypeParamsToParserNode::typeParams(const rbs_node_list_t *rbsTypeParams,
                                                   const RBSDeclaration &declaration) {
    parser::NodeVec result;
    result.reserve(rbsTypeParams->length);

    for (rbs_node_list_node_t *listNode = rbsTypeParams->head; listNode != nullptr; listNode = listNode->next) {
        auto *rbsTypeParam = rbs_down_cast<rbs_ast_type_param_t>(listNode->node);
        auto loc = declaration.typeLocFromRange(listNode->node->location->rg);

        if (rbsTypeParam->unchecked) {
            if (auto e = ctx.beginIndexerError(loc, core::errors::Rewriter::RBSUnsupported)) {
                e.setHeader("`{}` type parameters are not supported by Sorbet", "unchecked");
            }
        }

        auto nameStr = parser.resolveConstant(rbsTypeParam->name);
        auto nameConstant = ctx.state.enterNameConstant(nameStr);

        auto args = parser::NodeVec();
        if (rbsTypeParam->variance) {
            auto variance = parser.resolveKeyword(rbsTypeParam->variance);
            if (variance == "covariant") {
                args.emplace_back(parser::MK::Symbol(loc, core::Names::covariant()));
            } else if (variance == "contravariant") {
                args.emplace_back(parser::MK::Symbol(loc, core::Names::contravariant()));
            }
        }

        auto typeConst = parser::MK::Const(loc, nullptr, nameConstant);
        auto typeSend =
            parser::MK::Send(loc, parser::MK::SorbetPrivateStatic(loc), core::Names::typeMember(), loc, move(args));

        auto defaultType = rbsTypeParam->default_type;
        auto upperBound = rbsTypeParam->upper_bound;
        auto lowerBound = rbsTypeParam->lower_bound;

        if (defaultType || upperBound || lowerBound) {
            auto typeTranslator = TypeToParserNode(ctx, {}, parser);
            auto pairs = parser::NodeVec();

            if (defaultType) {
                pairs.emplace_back(make_unique<parser::Pair>(loc, parser::MK::Symbol(loc, core::Names::fixed()),
                                                             typeTranslator.toParserNode(defaultType, declaration)));
            }

            if (upperBound) {
                pairs.emplace_back(make_unique<parser::Pair>(loc, parser::MK::Symbol(loc, core::Names::upper()),
                                                             typeTranslator.toParserNode(upperBound, declaration)));
            }

            if (lowerBound) {
                pairs.emplace_back(make_unique<parser::Pair>(loc, parser::MK::Symbol(loc, core::Names::lower()),
                                                             typeTranslator.toParserNode(lowerBound, declaration)));
            }

            auto body = parser::MK::Hash(loc, false, move(pairs));
            typeSend = make_unique<parser::Block>(loc, move(typeSend), nullptr, move(body));
        }

        auto assign = make_unique<parser::Assign>(loc, move(typeConst), move(typeSend));
        result.emplace_back(move(assign));
    }

    return result;
}

} // namespace sorbet::rbs
