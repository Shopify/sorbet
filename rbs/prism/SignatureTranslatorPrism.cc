#include "rbs/prism/SignatureTranslatorPrism.h"
#include "core/errors/rewriter.h"
#include "parser/prism/Helpers.h"
#include "rbs/MethodTypeToParserNode.h"
#include "rbs/TypeParamsToParserNodes.h"
#include "rbs/TypeToParserNode.h"
#include "rbs/prism/MethodTypeToParserNodePrism.h"
#include "rbs/prism/TypeParamsToParserNodesPrism.h"
#include "rbs/prism/TypeToParserNodePrism.h"
#include "rbs/rbs_common.h"

using namespace std;
using namespace sorbet::parser::Prism;

namespace sorbet::rbs {

pm_node_t *
SignatureTranslatorPrism::translateAssertionType(absl::Span<pair<core::LocOffsets, core::NameRef>> typeParams,
                                                 const rbs::RBSDeclaration &assertion) {
    rbs_string_t rbsString = makeRBSString(assertion.string);
    const rbs_encoding_t *encoding = RBS_ENCODING_UTF_8_ENTRY;

    Parser parser(rbsString, encoding);
    rbs_node_t *rbsType = parser.parseType();

    if (parser.hasError()) {
        core::LocOffsets loc = assertion.typeLocFromRange(parser.getError()->token.range);
        if (auto e = ctx.beginIndexerError(loc, core::errors::Rewriter::RBSSyntaxError)) {
            e.setHeader("Failed to parse RBS type ({})", parser.getError()->message);
        }
        return nullptr;
    }

    auto typeToParserNodePrism = TypeToParserNodePrism(ctx, typeParams, move(parser), *this->parser);
    return typeToParserNodePrism.toPrismNode(rbsType, assertion);
}

pm_node_t *SignatureTranslatorPrism::translateType(const RBSDeclaration &declaration) {
    rbs_string_t rbsString = makeRBSString(declaration.string);
    const rbs_encoding_t *encoding = RBS_ENCODING_UTF_8_ENTRY;

    Parser parser(rbsString, encoding);
    rbs_node_t *rbsType = parser.parseType();

    if (parser.hasError()) {
        core::LocOffsets offset = declaration.typeLocFromRange(parser.getError()->token.range);
        if (auto e = ctx.beginIndexerError(offset, core::errors::Rewriter::RBSSyntaxError)) {
            e.setHeader("Failed to parse RBS type ({})", parser.getError()->message);
        }

        return nullptr;
    }

    absl::Span<pair<core::LocOffsets, core::NameRef>> emptyTypeParams;
    auto typeTranslator = TypeToParserNodePrism(ctx, emptyTypeParams, move(parser), *this->parser);
    return typeTranslator.toPrismNode(rbsType, declaration);
}

pm_node_t *SignatureTranslatorPrism::translateAttrSignature(const pm_call_node_t *call,
                                                            const RBSDeclaration &declaration,
                                                            absl::Span<const Comment> annotations) {
    rbs_string_t rbsString = makeRBSString(declaration.string);
    const rbs_encoding_t *encoding = RBS_ENCODING_UTF_8_ENTRY;

    Parser parser(rbsString, encoding);
    rbs_node_t *rbsType = parser.parseType();

    if (parser.hasError()) {
        core::LocOffsets offset = declaration.typeLocFromRange(parser.getError()->token.range);
        // First parse failed, let's check if the user mistakenly used a method signature on an accessor
        auto methodParser = Parser(rbsString, encoding);
        methodParser.parseMethodType();

        if (!methodParser.hasError()) {
            if (auto e = ctx.beginIndexerError(offset, core::errors::Rewriter::RBSSyntaxError)) {
                e.setHeader("Using a method signature on an accessor is not allowed, use a bare type instead");
            }
        } else {
            if (auto e = ctx.beginIndexerError(offset, core::errors::Rewriter::RBSSyntaxError)) {
                e.setHeader("Failed to parse RBS type ({})", methodParser.getError()->message);
            }
        }

        return nullptr;
    }

    auto methodTypeToParserNode = MethodTypeToParserNodePrism(ctx, move(parser), *this->parser);
    return methodTypeToParserNode.attrSignature(call, rbsType, declaration, annotations);
}

pm_node_t *SignatureTranslatorPrism::translateMethodSignature(const pm_node_t *methodDef,
                                                              const RBSDeclaration &declaration,
                                                              absl::Span<const Comment> annotations) {
    rbs_string_t rbsString = makeRBSString(declaration.string);
    const rbs_encoding_t *encoding = RBS_ENCODING_UTF_8_ENTRY;

    Parser parser(rbsString, encoding);
    rbs_method_type_t *rbsMethodType = parser.parseMethodType();

    if (parser.hasError()) {
        rbs_range_t tokenRange = parser.getError()->token.range;
        core::LocOffsets offset = declaration.typeLocFromRange(tokenRange);

        if (auto e = ctx.beginIndexerError(offset, core::errors::Rewriter::RBSSyntaxError)) {
            e.setHeader("Failed to parse RBS signature ({})", parser.getError()->message);
        }

        return nullptr;
    }

    auto methodTypeToParserNodePrism = MethodTypeToParserNodePrism(ctx, move(parser), *this->parser);
    return methodTypeToParserNodePrism.methodSignature(methodDef, rbsMethodType, declaration, annotations);
}

vector<pm_node_t *> SignatureTranslatorPrism::translateTypeParams(const RBSDeclaration &declaration) {
    rbs_string_t rbsString = makeRBSString(declaration.string);
    const rbs_encoding_t *encoding = RBS_ENCODING_UTF_8_ENTRY;

    Parser rbsParser(rbsString, encoding);
    rbs_node_list_t *rbsTypeParams = rbsParser.parseTypeParams();

    if (rbsParser.hasError()) {
        rbs_range_t tokenRange = rbsParser.getError()->token.range;
        core::LocOffsets offset = declaration.typeLocFromRange(tokenRange);

        if (auto e = ctx.beginIndexerError(offset, core::errors::Rewriter::RBSSyntaxError)) {
            e.setHeader("Failed to parse RBS type parameters ({})", rbsParser.getError()->message);
        }

        return vector<pm_node_t *>{};
    }

    auto typeParamsToParserNode = TypeParamsToParserNodePrism(ctx, move(rbsParser), *parser);
    return typeParamsToParserNode.typeParams(rbsTypeParams, declaration);
}

} // namespace sorbet::rbs
