#include "rbs/prism/CommentsAssociatorPrism.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "core/errors/rewriter.h"
#include "parser/helper.h"
#include "parser/prism/Helpers.h"
#include "rbs/SignatureTranslator.h"

using namespace std;
using namespace sorbet::parser::Prism;

namespace sorbet::rbs {

const string_view CommentsAssociatorPrism::RBS_PREFIX = "#:";
const string_view CommentsAssociatorPrism::ANNOTATION_PREFIX = "# @";
const string_view CommentsAssociatorPrism::MULTILINE_RBS_PREFIX = "#|";
const string_view CommentsAssociatorPrism::BIND_PREFIX = "#: self as ";

namespace {

// Static regex patterns to avoid recompilation
const regex TYPE_ALIAS_PATTERN_PRISM("^#: type\\s*([a-z][A-Za-z0-9_]*)\\s*=\\s*([^\\n]*)$");
const regex HEREDOC_PATTERN_PRISM("\\s*=?\\s*<<(-|~)[^,\\s\\n#]+(,\\s*<<(-|~)[^,\\s\\n#]+)*");

constexpr std::string_view TYPE_ALIAS_PREFIX = "type ";

/**
 * Create a synthetic placeholder node (PM_CONSTANT_READ_NODE) with a marker constant ID.
 * Used to mark locations for bind comments and type aliases.
 */
pm_node_t *createSyntheticPlaceholder(sorbet::parser::Prism::Parser &parser, const CommentNodePrism &comment,
                                      pm_constant_id_t marker) {
    sorbet::parser::Prism::Factory factory{parser};
    return factory.ConstantReadNode(marker, comment.loc);
}

/**
 * Insert a node into a pm_node_list_t at a specific index.
 * The node is first appended to the end, then shifted into position.
 */
void insertNodeAtIndex(pm_node_list_t &nodes, pm_node_t *node, size_t index) {
    pm_node_list_append(&nodes, node);
    for (size_t i = nodes.size - 1; i > index; i--) {
        nodes.nodes[i] = nodes.nodes[i - 1];
    }
    nodes.nodes[index] = node;
}

/**
 * Check if the given range is the start of a heredoc assignment `= <<~FOO` and return the position of the end of the
 * heredoc marker.
 *
 * Returns nullopt if no heredoc marker is found.
 */
optional<uint32_t> hasHeredocMarker(const core::Context &ctx, uint32_t fromPos, uint32_t toPos) {
    string_view source(ctx.file.data(ctx).source().substr(fromPos, toPos - fromPos));

    string sourceStr(source);
    smatch match;
    if (regex_search(sourceStr, HEREDOC_PATTERN_PRISM)) {
        return fromPos + sourceStr.length();
    }

    return nullopt;
}
} // namespace

optional<uint32_t> CommentsAssociatorPrism::locateTargetLine(pm_node_t *node) {
    optional<uint32_t> result = nullopt;

    if (node == nullptr) {
        return result;
    }

    switch (PM_NODE_TYPE(node)) {
        case PM_STRING_NODE: {
            auto loc = translateLocation(node->location);
            if (hasHeredocMarker(ctx, loc.beginPos(), loc.endPos())) {
                result = core::Loc::pos2Detail(ctx.file.data(ctx), loc.beginPos()).line;
            }
            break;
        }
        case PM_INTERPOLATED_STRING_NODE: {
            auto loc = translateLocation(node->location);
            if (hasHeredocMarker(ctx, loc.beginPos(), loc.endPos())) {
                result = core::Loc::pos2Detail(ctx.file.data(ctx), loc.beginPos()).line;
            }
            break;
        }
        case PM_ARRAY_NODE: {
            auto *arr = down_cast<pm_array_node_t>(node);
            for (size_t i = 0; i < arr->elements.size; i++) {
                if (auto line = locateTargetLine(arr->elements.nodes[i])) {
                    result = *line;
                    break;
                }
            }
            break;
        }
        case PM_CALL_NODE: {
            auto *call = down_cast<pm_call_node_t>(node);
            result = locateTargetLine(call->receiver);
            break;
        }
        default: {
            // No special handling for other node types
            break;
        }
    }

    return result;
}

core::LocOffsets CommentsAssociatorPrism::translateLocation(pm_location_t location) {
    const uint8_t *sourceStart = (const uint8_t *)ctx.file.data(ctx).source().data();
    uint32_t start = static_cast<uint32_t>(location.start - sourceStart);
    uint32_t end = static_cast<uint32_t>(location.end - sourceStart);
    return core::LocOffsets{start, end};
}

void CommentsAssociatorPrism::consumeCommentsInsideNode(pm_node_t *node, string_view kind) {
    auto loc = translateLocation(node->location);
    auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), loc.beginPos()).line;
    auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), loc.endPos()).line;
    consumeCommentsBetweenLines(beginLine, endLine, kind);
}

void CommentsAssociatorPrism::consumeCommentsBetweenLines(int startLine, int endLine, string_view kind) {
    auto it = commentByLine.begin();
    while (it != commentByLine.end() && it->first < startLine) {
        ++it;
    }
    if (it == commentByLine.end()) {
        return;
    }
    auto startIt = it;

    while (it != commentByLine.end()) {
        if (it->first < endLine) {
            if (absl::StartsWith(it->second.string, RBS_PREFIX) ||
                absl::StartsWith(it->second.string, MULTILINE_RBS_PREFIX)) {
                if (it->first == startLine) {
                    if (auto e = ctx.beginError(it->second.loc, core::errors::Rewriter::RBSUnusedComment)) {
                        e.setHeader("Unexpected RBS assertion comment found after `{}` declaration", kind);
                    }
                } else {
                    if (auto e = ctx.beginError(it->second.loc, core::errors::Rewriter::RBSUnusedComment)) {
                        e.setHeader("Unexpected RBS assertion comment found in `{}`", kind);
                    }
                }
            }
        } else if (it->first == endLine) {
            if (absl::StartsWith(it->second.string, RBS_PREFIX) ||
                absl::StartsWith(it->second.string, MULTILINE_RBS_PREFIX)) {
                if (auto e = ctx.beginError(it->second.loc, core::errors::Rewriter::RBSAssertionError)) {
                    e.setHeader("Unexpected RBS assertion comment found after `{}` end", kind);
                }
            }
        } else {
            break;
        }
        ++it;
    }
    commentByLine.erase(startIt, it);
}

void CommentsAssociatorPrism::consumeCommentsUntilLine(int line) {
    auto it = commentByLine.begin();
    while (it != commentByLine.end()) {
        if (it->first < line) {
            if (absl::StartsWith(it->second.string, RBS_PREFIX) ||
                absl::StartsWith(it->second.string, MULTILINE_RBS_PREFIX)) {
                if (auto e = ctx.beginIndexerError(it->second.loc, core::errors::Rewriter::RBSUnusedComment)) {
                    e.setHeader("Unused RBS signature comment. No method definition found after it");
                }
            }
            ++it;
        } else {
            break;
        }
    }
    commentByLine.erase(commentByLine.begin(), it);
}

void CommentsAssociatorPrism::associateAssertionCommentsToNode(pm_node_t *node, bool adjustLocForHeredoc) {
    auto loc = translateLocation(node->location);
    uint32_t targetLine = core::Loc::pos2Detail(ctx.file.data(ctx), loc.endPos()).line;
    if (adjustLocForHeredoc) {
        if (auto line = locateTargetLine(node)) {
            targetLine = *line;
        }
    }

    vector<CommentNodePrism> comments;

    auto it = commentByLine.find(targetLine);
    if (it != commentByLine.end() && absl::StartsWith(it->second.string, RBS_PREFIX)) {
        comments.emplace_back(it->second);
        commentByLine.erase(it);

        assertionsForNode[node] = move(comments);
    }
}

void CommentsAssociatorPrism::associateSignatureCommentsToNode(pm_node_t *node) {
    auto loc = translateLocation(node->location);
    auto nodeStartLine = core::Loc::pos2Detail(ctx.file.data(ctx), loc.beginPos()).line;

    vector<CommentNodePrism> comments;

    for (auto it = commentByLine.begin(); it != commentByLine.end();) {
        auto commentLine = it->first;

        if (commentLine >= nodeStartLine) {
            break;
        }

        if (absl::StartsWith(it->second.string, RBS_PREFIX) ||
            absl::StartsWith(it->second.string, MULTILINE_RBS_PREFIX) ||
            absl::StartsWith(it->second.string, ANNOTATION_PREFIX)) {
            comments.emplace_back(it->second);
            it = commentByLine.erase(it);
            continue;
        }

        it++;
    }

    signaturesForNode[node] = move(comments);
}

int CommentsAssociatorPrism::maybeInsertStandalonePlaceholders(pm_node_list_t &nodes, int index, int lastLine,
                                                               int currentLine) {
    if (lastLine == currentLine) {
        return 0;
    }

    auto inserted = 0;
    pm_node_t *continuationFor = nullptr;

    for (auto it = commentByLine.begin(); it != commentByLine.end();) {
        if (it->first <= lastLine) {
            it++;
            continue;
        }

        if (it->first >= currentLine) {
            break;
        }

        if (continuationFor && absl::StartsWith(it->second.string, MULTILINE_RBS_PREFIX)) {
            signaturesForNode[continuationFor].emplace_back(move(it->second));
            // Note: can't extend location on pm_node_t easily, skip for now
            it = commentByLine.erase(it);
            continue;
        }

        // Handle bind comments: `#: self as Type`
        // Creates a synthetic placeholder node that will be replaced with T.bind(self, Type)
        if (absl::StartsWith(it->second.string, BIND_PREFIX)) {
            continuationFor = nullptr;

            // Create placeholder node with special marker constant ID
            pm_node_t *placeholder = createSyntheticPlaceholder(parser, it->second, RBS_SYNTHETIC_BIND_MARKER);

            // Register comment for later processing by AssertionsRewriter
            assertionsForNode[placeholder] = vector<CommentNodePrism>{it->second};
            it = commentByLine.erase(it);

            // Insert placeholder into statement list
            insertNodeAtIndex(nodes, placeholder, index);
            inserted++;

            continue;
        }

        // Handle type alias comments: `#: type foo = String`
        // Creates a constant assignment: `type foo = T.type_alias { String }`
        // This allows using `foo` as a type alias later (e.g., `#: self as foo`)
        std::smatch matches;
        auto str = string(it->second.string);
        if (std::regex_match(str, matches, TYPE_ALIAS_PATTERN_PRISM)) {
            // Type aliases are only allowed in class/module bodies
            if (!contextAllowingTypeAlias.empty()) {
                if (auto [allow, loc] = contextAllowingTypeAlias.back(); !allow) {
                    if (auto e = ctx.beginError(it->second.loc, core::errors::Rewriter::RBSUnusedComment)) {
                        e.setHeader("Unexpected RBS type alias comment");
                        e.addErrorLine(ctx.locAt(loc),
                                       "RBS type aliases are only allowed in class and module bodies. Found in:");
                    }

                    it = commentByLine.erase(it);
                    continue;
                }
            }

            // Register the constant name (e.g., "type foo") in the symbol table
            auto nameStr = std::string(TYPE_ALIAS_PREFIX) + matches[1].str();
            ctx.state.enterNameConstant(nameStr);

            // Create placeholder for the type expression
            // This will be replaced with T.type_alias { Type } by SigsRewriter
            pm_node_t *placeholder = createSyntheticPlaceholder(parser, it->second, RBS_SYNTHETIC_TYPE_ALIAS_MARKER);

            // Register comment for later processing by SigsRewriter
            signaturesForNode[placeholder] = vector<CommentNodePrism>{it->second};

            continuationFor = placeholder;

            // Create constant assignment node: `type foo = placeholder`
            pm_constant_id_t name_id = prism.addConstantToPool(nameStr.c_str());
            pm_node_t *constantWrite = prism.ConstantWriteNode(it->second.loc, name_id, placeholder);

            // Insert the assignment into the statement list
            insertNodeAtIndex(nodes, constantWrite, index);
            inserted++;
            it = commentByLine.erase(it);
            continue;
        }

        continuationFor = nullptr;

        it++;
    }

    return inserted;
}

void CommentsAssociatorPrism::walkConditionalNode(pm_node_t *node, pm_node_t *predicate,
                                                  pm_statements_node_t *&statements, pm_node_t *&elsePart,
                                                  std::string_view kind) {
    auto nodeLoc = translateLocation(node->location);
    auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), nodeLoc.beginPos()).line;
    auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), nodeLoc.endPos()).line;

    if (beginLine == endLine) {
        associateAssertionCommentsToNode(node);
    }

    walkNode(predicate);

    lastLine = core::Loc::pos2Detail(ctx.file.data(ctx), nodeLoc.beginPos()).line;

    statements = down_cast<pm_statements_node_t>(walkBody(node, up_cast(statements)));

    if (statements) {
        auto stmtLoc = translateLocation(statements->base.location);
        lastLine = core::Loc::pos2Detail(ctx.file.data(ctx), stmtLoc.endPos()).line;
    }

    elsePart = walkBody(node, elsePart);

    if (beginLine != endLine) {
        associateAssertionCommentsToNode(node);
    }

    consumeCommentsInsideNode(node, kind);
}

pm_node_t *CommentsAssociatorPrism::walkBody(pm_node_t *node, pm_node_t *body) {
    if (body == nullptr) {
        return nullptr;
    }

    if (PM_NODE_TYPE_P(body, PM_BEGIN_NODE)) {
        // The body is already a Begin node, so we don't need any wrapping
        auto *begin = down_cast<pm_begin_node_t>(body);
        walkNode(body);

        // Visit standalone RBS comments after the last node in the body
        if (auto *statements = begin->statements) {
            auto loc = translateLocation(node->location);
            int endLine = core::Loc::pos2Detail(ctx.file.data(ctx), loc.endPos()).line;
            maybeInsertStandalonePlaceholders(statements->body, 0, lastLine, endLine);
            lastLine = endLine;
        }

        return body;
    }

    if (PM_NODE_TYPE_P(body, PM_STATEMENTS_NODE)) {
        walkNode(body);

        return body;
    }

    // The body is a single node, we'll need to wrap it if we find standalone RBS comments
    pm_node_list_t beforeNodes = {0, 0, NULL};

    // Visit standalone RBS comments before the body node
    auto bodyLoc = translateLocation(body->location);
    auto currentLine = core::Loc::pos2Detail(ctx.file.data(ctx), bodyLoc.beginPos()).line;
    maybeInsertStandalonePlaceholders(beforeNodes, 0, lastLine, currentLine);
    lastLine = currentLine;

    walkNode(body);

    // Visit standalone RBS comments after the body node
    pm_node_list_t afterNodes = {0, 0, NULL};
    auto loc = translateLocation(node->location);
    int endLine = core::Loc::pos2Detail(ctx.file.data(ctx), loc.endPos()).line;
    maybeInsertStandalonePlaceholders(afterNodes, 0, lastLine, endLine);
    lastLine = endLine;

    if (beforeNodes.size > 0 || afterNodes.size > 0) {
        pm_node_list_t nodes = {0, 0, NULL};

        // Add before nodes
        for (size_t i = 0; i < beforeNodes.size; i++) {
            pm_node_list_append(&nodes, beforeNodes.nodes[i]);
        }

        // Add the body node
        pm_node_list_append(&nodes, body);

        // Add after nodes
        for (size_t i = 0; i < afterNodes.size; i++) {
            pm_node_list_append(&nodes, afterNodes.nodes[i]);
        }

        // TODO: Create proper PM_BEGIN_NODE with combined nodes
        // For now, return the original body until proper Begin node creation is implemented
        // Need to free the temporary node list
        pm_node_list_free(&nodes);
        pm_node_list_free(&beforeNodes);
        pm_node_list_free(&afterNodes);
        return body;
    }

    // Free temporary node lists even when not used
    pm_node_list_free(&beforeNodes);
    pm_node_list_free(&afterNodes);
    return body;
}

template <typename PrismNode> void CommentsAssociatorPrism::walkAssignmentNode(pm_node_t *untypedNode) {
    auto *assign = down_cast<PrismNode>(untypedNode);
    associateAssertionCommentsToNode(assign->value, true);
    walkNode(assign->value);
    consumeCommentsInsideNode(untypedNode, "assign");
}

template <typename PrismNode>
void CommentsAssociatorPrism::walkCallAssignmentNode(pm_node_t *untypedNode, std::string_view label) {
    auto *assign = down_cast<PrismNode>(untypedNode);
    associateAssertionCommentsToNode(assign->value, true);
    walkNode(assign->value);
    walkNode(assign->receiver);
    consumeCommentsInsideNode(untypedNode, label);
}

template <typename PrismNode>
void CommentsAssociatorPrism::walkIndexAssignmentNode(pm_node_t *untypedNode, std::string_view label) {
    auto *assign = down_cast<PrismNode>(untypedNode);
    associateAssertionCommentsToNode(assign->value, true);
    walkNode(assign->receiver);
    if (assign->arguments != nullptr) {
        walkNodes(assign->arguments->arguments);
    }
    walkNode(up_cast(assign->block));
    walkNode(assign->value);
    consumeCommentsInsideNode(untypedNode, label);
}

void CommentsAssociatorPrism::walkNodes(pm_node_list_t &nodes) {
    for (size_t i = 0; i < nodes.size; i++) {
        auto node = nodes.nodes[i];
        walkNode(node);
    }
}

void CommentsAssociatorPrism::walkStatements(pm_node_list_t &nodes) {
    for (size_t i = 0; i < nodes.size; i++) {
        auto *stmt = nodes.nodes[i];

        if (PM_NODE_TYPE_P(stmt, PM_ENSURE_NODE)) {
            // Ensure need to be visited handled differently because of how we desugar their structure.
            // The bind needs to be added _inside_ them and not before if we want the type to be applied
            // properly.
            walkNode(stmt);
            continue;
        }

        auto stmtLoc = translateLocation(stmt->location);
        auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), stmtLoc.beginPos()).line;

        auto inserted = maybeInsertStandalonePlaceholders(nodes, i, lastLine, beginLine);
        i += inserted;

        walkNode(stmt);

        lastLine = core::Loc::pos2Detail(ctx.file.data(ctx), stmtLoc.endPos()).line;
    }
}

void CommentsAssociatorPrism::walkNode(pm_node_t *node) {
    if (node == nullptr) {
        return;
    }

    switch (PM_NODE_TYPE(node)) {
        case PM_AND_NODE: {
            auto *and_ = down_cast<pm_and_node_t>(node);
            associateAssertionCommentsToNode(node);
            walkNode(and_->right);
            walkNode(and_->left);
            consumeCommentsInsideNode(node, "and");
            break;
        }
        case PM_ARRAY_NODE: {
            auto *array = down_cast<pm_array_node_t>(node);
            associateAssertionCommentsToNode(node);
            walkNodes(array->elements);
            consumeCommentsInsideNode(node, "array");
            break;
        }
        case PM_BEGIN_NODE: {
            auto *begin = down_cast<pm_begin_node_t>(node);
            // Differentiate between implicit and explicit begin nodes by checking if begin_keyword_loc is populated
            // In Prism, both implicit and explicit begin constructs use PM_BEGIN_NODE, unlike Whitequark which had
            // separate types
            bool isExplicitBegin = begin->begin_keyword_loc.start != begin->begin_keyword_loc.end;

            if (isExplicitBegin) {
                // Explicit begin...end block (was kwbegin in Whitequark)
                associateAssertionCommentsToNode(node);
            } else {
                // Implicit begin node (wrapping expressions)
                // This is a workaround that will be removed once we migrate to prism. We need to differentiate
                // between implicit and explicit begin nodes.
                //
                // (let4 &&= "foo") #: Integer
                // vs
                // take_block { |x|
                //   puts x
                //   x #: as String
                // }
                if (begin->statements && begin->statements->body.size > 0) {
                    auto firstStmtLoc = translateLocation(begin->statements->body.nodes[0]->location);
                    auto nodeLoc = translateLocation(node->location);
                    if (firstStmtLoc.endPos() + 1 == nodeLoc.endPos()) {
                        associateAssertionCommentsToNode(node);
                    }
                }
            }

            if (begin->statements) {
                walkStatements(begin->statements->body);
            }

            walkNode(up_cast(begin->rescue_clause));
            walkNode(up_cast(begin->else_clause));
            walkNode(up_cast(begin->ensure_clause));

            auto nodeLoc = translateLocation(node->location);
            lastLine = core::Loc::pos2Detail(ctx.file.data(ctx), nodeLoc.endPos()).line;
            consumeCommentsInsideNode(node, "begin");
            break;
        }
        case PM_BLOCK_NODE: {
            auto *block = down_cast<pm_block_node_t>(node);
            auto blockLoc = translateLocation(node->location);
            auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), blockLoc.beginPos()).line;
            consumeCommentsUntilLine(beginLine);

            associateAssertionCommentsToNode(node);

            block->body = walkBody(node, block->body);
            auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), blockLoc.endPos()).line;
            consumeCommentsBetweenLines(beginLine, endLine, "block");
            break;
        }
        case PM_LAMBDA_NODE: {
            auto *lambda = down_cast<pm_lambda_node_t>(node);
            auto lambdaLoc = translateLocation(node->location);
            auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), lambdaLoc.beginPos()).line;
            associateAssertionCommentsToNode(node);

            lambda->body = walkBody(node, lambda->body);
            auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), lambdaLoc.endPos()).line;
            consumeCommentsBetweenLines(beginLine, endLine, "lambda");
            break;
        }
        case PM_BREAK_NODE: {
            auto *break_ = down_cast<pm_break_node_t>(node);
            // Only associate comments if the last expression is on the same line as the break
            if (break_->arguments != nullptr && break_->arguments->arguments.size > 0) {
                auto breakLoc = translateLocation(node->location);
                auto breakLine = core::Loc::pos2Detail(ctx.file.data(ctx), breakLoc.beginPos()).line;
                auto lastArgIdx = break_->arguments->arguments.size - 1;
                auto lastArgLoc = translateLocation(break_->arguments->arguments.nodes[lastArgIdx]->location);
                auto lastExprLine = core::Loc::pos2Detail(ctx.file.data(ctx), lastArgLoc.beginPos()).line;
                if (lastExprLine == breakLine) {
                    associateAssertionCommentsToNode(node);
                }
            }

            if (break_->arguments != nullptr) {
                walkNodes(break_->arguments->arguments);
            }
            consumeCommentsInsideNode(node, "break");
            break;
        }
        case PM_CASE_NODE: {
            auto *case_ = down_cast<pm_case_node_t>(node);
            associateAssertionCommentsToNode(node);
            walkNode(case_->predicate);
            walkNodes(case_->conditions);
            case_->else_clause = down_cast<pm_else_node_t>(walkBody(node, up_cast(case_->else_clause)));
            consumeCommentsInsideNode(node, "case");
            break;
        }
        case PM_CASE_MATCH_NODE: {
            auto *case_ = down_cast<pm_case_match_node_t>(node);
            associateAssertionCommentsToNode(node);
            walkNode(case_->predicate);
            walkNodes(case_->conditions);
            case_->else_clause = down_cast<pm_else_node_t>(walkBody(node, up_cast(case_->else_clause)));
            consumeCommentsInsideNode(node, "case");
            break;
        }
        case PM_CLASS_NODE: {
            auto *cls = down_cast<pm_class_node_t>(node);
            auto classKeywordLoc = translateLocation(cls->class_keyword_loc);
            contextAllowingTypeAlias.push_back(make_pair(true, classKeywordLoc));
            associateSignatureCommentsToNode(node);
            auto classLoc = translateLocation(node->location);
            auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), classLoc.beginPos()).line;
            consumeCommentsUntilLine(beginLine);
            cls->body = walkBody(up_cast(cls), cls->body);
            auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), classLoc.endPos()).line;
            consumeCommentsBetweenLines(beginLine, endLine, "class");
            contextAllowingTypeAlias.pop_back();
            break;
        }
        case PM_CALL_NODE: {
            auto *call = down_cast<pm_call_node_t>(node);

            if (parser.isVisibilityCall(node)) {
                // This is a visibility modifier wrapping a method definition: `private def foo; end`
                associateSignatureCommentsToNode(node);
                associateAssertionCommentsToNode(node);
                walkNode(call->receiver);
                if (call->arguments != nullptr) {
                    walkNodes(call->arguments->arguments);
                }
                consumeCommentsInsideNode(node, "call");
            } else if (parser.isAttrAccessorCall(node)) {
                associateSignatureCommentsToNode(node);
                associateAssertionCommentsToNode(node);
                walkNode(call->receiver);
                if (call->arguments != nullptr) {
                    walkNodes(call->arguments->arguments);
                }
                consumeCommentsInsideNode(node, "call");
            } else if (call->arguments != nullptr && call->arguments->arguments.size == 1 &&
                       parser.isSafeNavigationCall(node) && parser.isSetterCall(node)) {
                // Handle safe navigation setter calls: `foo&.bar = val #: Type`
                associateAssertionCommentsToNode(call->arguments->arguments.nodes[0]);
                walkNode(call->arguments->arguments.nodes[0]);
                consumeCommentsInsideNode(node, "safe navigation call");
            } else if (parser.resolveConstant(call->name) == "[]=" || parser.isSetterCall(node)) {
                // This is an assign through a call, either: `foo[key]=(y)` or `foo.x=(y)`
                //
                // Note: the parser groups the args on the right hand side of the assignment into an array node:
                //  * for `foo.x = 1, 2` the args are `[1, 2]`
                //  * for `foo[k1, k2] = 1, 2` the args are `[k1, k2, [1, 2]]`
                //
                // We always apply the cast starting from the last arg by walking them in reverse order.
                if (call->arguments != nullptr) {
                    for (int i = call->arguments->arguments.size - 1; i >= 0; i--) {
                        walkNode(call->arguments->arguments.nodes[i]);
                    }
                }
                walkNode(call->receiver);
                consumeCommentsInsideNode(node, "call");
            } else {
                associateAssertionCommentsToNode(node);
                walkNode(call->receiver);
                if (call->arguments != nullptr) {
                    walkNodes(call->arguments->arguments);
                }
                walkNode(call->block);
            }
            break;
        }
        case PM_CALL_AND_WRITE_NODE: {
            walkCallAssignmentNode<pm_call_and_write_node_t>(node, "and_asgn");
            break;
        }
        case PM_CALL_OR_WRITE_NODE: {
            walkCallAssignmentNode<pm_call_or_write_node_t>(node, "or_asgn");
            break;
        }
        case PM_CALL_OPERATOR_WRITE_NODE: {
            walkCallAssignmentNode<pm_call_operator_write_node_t>(node, "op_asgn");
            break;
        }
        case PM_INDEX_AND_WRITE_NODE: {
            walkIndexAssignmentNode<pm_index_and_write_node_t>(node, "and_asgn");
            break;
        }
        case PM_INDEX_OR_WRITE_NODE: {
            walkIndexAssignmentNode<pm_index_or_write_node_t>(node, "or_asgn");
            break;
        }
        case PM_INDEX_OPERATOR_WRITE_NODE: {
            walkIndexAssignmentNode<pm_index_operator_write_node_t>(node, "op_asgn");
            break;
        }
        case PM_DEF_NODE: {
            auto *def = down_cast<pm_def_node_t>(node);
            auto defKeywordLoc = translateLocation(def->def_keyword_loc);
            contextAllowingTypeAlias.push_back(make_pair(false, defKeywordLoc));
            associateSignatureCommentsToNode(node);

            // This is a singleton method definition (def obj.method) if receiver exists
            walkNode(def->receiver);

            def->body = walkBody(up_cast(def), def->body);
            consumeCommentsInsideNode(node, "method");
            contextAllowingTypeAlias.pop_back();
            break;
        }
        case PM_ELSE_NODE: {
            auto *else_ = down_cast<pm_else_node_t>(node);
            walkNode(up_cast(else_->statements));
            consumeCommentsInsideNode(node, "else");
            break;
        }
        case PM_ENSURE_NODE: {
            auto *ensure = down_cast<pm_ensure_node_t>(node);
            walkNode(up_cast(ensure->statements));
            consumeCommentsInsideNode(node, "ensure");
            break;
        }
        case PM_FOR_NODE: {
            auto *for_ = down_cast<pm_for_node_t>(node);
            associateAssertionCommentsToNode(node);
            walkNode(for_->collection);
            walkNode(for_->index);
            walkNode(up_cast(for_->statements));
            consumeCommentsInsideNode(node, "for");
            break;
        }
        case PM_HASH_NODE: {
            auto *hash = down_cast<pm_hash_node_t>(node);
            // TODO: Check if this hash is keyword arguments or regular hash
            // For now, always associate assertion comments
            associateAssertionCommentsToNode(node);
            walkNodes(hash->elements);
            consumeCommentsInsideNode(node, "hash");
            break;
        }
        case PM_IF_NODE: {
            auto *if_ = down_cast<pm_if_node_t>(node);
            walkConditionalNode(node, if_->predicate, if_->statements, if_->subsequent, "if");
            break;
        }
        case PM_UNLESS_NODE: {
            auto *unless_ = down_cast<pm_unless_node_t>(node);
            pm_node_t *elseClause = up_cast(unless_->else_clause);
            walkConditionalNode(node, unless_->predicate, unless_->statements, elseClause, "unless");
            unless_->else_clause = down_cast<pm_else_node_t>(elseClause);
            break;
        }
        case PM_IN_NODE: {
            auto *inPattern = down_cast<pm_in_node_t>(node);
            walkNode(inPattern->pattern);
            // Note: InNode doesn't have a guard field in Prism
            walkNode(up_cast(inPattern->statements));
            consumeCommentsInsideNode(node, "in_pattern");
            break;
        }
        case PM_KEYWORD_HASH_NODE: {
            auto *kwh = down_cast<pm_keyword_hash_node_t>(node);
            walkNodes(kwh->elements);
            consumeCommentsInsideNode(node, "kwsplat");
            break;
        }
        case PM_MULTI_WRITE_NODE: {
            auto *masgn = down_cast<pm_multi_write_node_t>(node);
            associateAssertionCommentsToNode(masgn->value, true);
            walkNode(masgn->value);
            consumeCommentsInsideNode(node, "masgn");
            break;
        }
        case PM_MODULE_NODE: {
            auto *mod = down_cast<pm_module_node_t>(node);
            auto moduleKeywordLoc = translateLocation(mod->module_keyword_loc);
            contextAllowingTypeAlias.push_back(make_pair(true, moduleKeywordLoc));
            associateSignatureCommentsToNode(node);
            auto modLoc = translateLocation(node->location);
            auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), modLoc.beginPos()).line;
            consumeCommentsUntilLine(beginLine);
            mod->body = walkBody(up_cast(mod), mod->body);
            auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), modLoc.endPos()).line;
            consumeCommentsBetweenLines(beginLine, endLine, "module");
            contextAllowingTypeAlias.pop_back();
            break;
        }
        case PM_NEXT_NODE: {
            auto *next = down_cast<pm_next_node_t>(node);
            // Only associate comments if the last expression is on the same line as the next
            if (next->arguments && next->arguments->arguments.size > 0) {
                auto nextLoc = translateLocation(node->location);
                auto nextLine = core::Loc::pos2Detail(ctx.file.data(ctx), nextLoc.beginPos()).line;
                auto lastArgIdx = next->arguments->arguments.size - 1;
                auto lastArgLoc = translateLocation(next->arguments->arguments.nodes[lastArgIdx]->location);
                auto lastExprLine = core::Loc::pos2Detail(ctx.file.data(ctx), lastArgLoc.beginPos()).line;
                if (lastExprLine == nextLine) {
                    associateAssertionCommentsToNode(node);
                }
            }

            if (next->arguments) {
                walkNodes(next->arguments->arguments);
            }
            consumeCommentsInsideNode(node, "next");
            break;
        }
        case PM_OR_NODE: {
            auto *or_ = down_cast<pm_or_node_t>(node);
            associateAssertionCommentsToNode(node);
            walkNode(or_->right);
            walkNode(or_->left);
            consumeCommentsInsideNode(node, "or");
            break;
        }
        case PM_ASSOC_NODE: {
            auto *pair = down_cast<pm_assoc_node_t>(node);
            walkNode(pair->value);
            walkNode(pair->key);
            consumeCommentsInsideNode(node, "pair");
            break;
        }
        case PM_RESCUE_NODE: {
            auto *rescue = down_cast<pm_rescue_node_t>(node);
            auto rescueLoc = translateLocation(node->location);
            auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), rescueLoc.beginPos()).line;
            auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), rescueLoc.endPos()).line;

            if (beginLine == endLine) {
                // Single line rescue that may have an assertion comment
                // Walk statements in the rescue body
                if (rescue->statements) {
                    walkNode(up_cast(rescue->statements));
                }
                // Walk subsequent rescue clauses
                if (rescue->subsequent) {
                    walkNode(up_cast(rescue->subsequent));
                }
            } else {
                // Multi-line rescue
                // Walk subsequent rescue clauses first for multi-line
                if (rescue->subsequent) {
                    walkNode(up_cast(rescue->subsequent));
                }
                // Walk statements in the rescue body
                if (rescue->statements) {
                    walkNode(up_cast(rescue->statements));
                }
            }

            consumeCommentsBetweenLines(beginLine, endLine, "rescue");
            break;
        }
        case PM_RESCUE_MODIFIER_NODE: {
            auto *rescueMod = down_cast<pm_rescue_modifier_node_t>(node);
            walkNode(rescueMod->rescue_expression);
            walkNode(rescueMod->expression);
            consumeCommentsInsideNode(node, "rescue");
            break;
        }
        case PM_RETURN_NODE: {
            auto *ret = down_cast<pm_return_node_t>(node);
            // Only associate comments if the last expression is on the same line as the return
            if (ret->arguments && ret->arguments->arguments.size > 0) {
                auto returnLoc = translateLocation(node->location);
                auto returnLine = core::Loc::pos2Detail(ctx.file.data(ctx), returnLoc.beginPos()).line;
                auto lastArgIdx = ret->arguments->arguments.size - 1;
                auto lastArgLoc = translateLocation(ret->arguments->arguments.nodes[lastArgIdx]->location);
                auto lastExprLine = core::Loc::pos2Detail(ctx.file.data(ctx), lastArgLoc.beginPos()).line;
                if (lastExprLine == returnLine) {
                    associateAssertionCommentsToNode(node);
                }
            }

            if (ret->arguments) {
                walkNodes(ret->arguments->arguments);
            }
            consumeCommentsInsideNode(node, "return");
            break;
        }
        case PM_SINGLETON_CLASS_NODE: {
            auto *sclass = down_cast<pm_singleton_class_node_t>(node);
            auto classKeywordLoc = translateLocation(sclass->class_keyword_loc);
            contextAllowingTypeAlias.push_back(make_pair(true, classKeywordLoc));
            associateSignatureCommentsToNode(node);
            auto sclassLoc = translateLocation(node->location);
            auto beginLine = core::Loc::pos2Detail(ctx.file.data(ctx), sclassLoc.beginPos()).line;
            consumeCommentsUntilLine(beginLine);
            sclass->body = walkBody(up_cast(sclass), sclass->body);
            auto endLine = core::Loc::pos2Detail(ctx.file.data(ctx), sclassLoc.endPos()).line;
            consumeCommentsBetweenLines(beginLine, endLine, "sclass");
            contextAllowingTypeAlias.pop_back();
            break;
        }
        case PM_SPLAT_NODE: {
            auto *splat = down_cast<pm_splat_node_t>(node);
            walkNode(splat->expression);
            consumeCommentsInsideNode(node, "splat");
            break;
        }
        case PM_ASSOC_SPLAT_NODE: {
            auto *splatAssoc = down_cast<pm_assoc_splat_node_t>(node);
            if (auto *value = splatAssoc->value) {
                associateAssertionCommentsToNode(value);
                walkNode(value);
            }
            break;
        }
        case PM_SUPER_NODE: {
            auto *super_ = down_cast<pm_super_node_t>(node);
            associateAssertionCommentsToNode(node);
            if (super_->arguments) {
                walkNodes(super_->arguments->arguments);
            }
            walkNode(super_->block);
            consumeCommentsInsideNode(node, "super");
            break;
        }
        case PM_UNTIL_NODE: {
            auto *until = down_cast<pm_until_node_t>(node);
            // Check if this is post-condition until (modifier form: "body until condition")
            bool isModifier = until->base.flags & PM_LOOP_FLAGS_BEGIN_MODIFIER;

            if (isModifier) {
                // Post until: body executes first, then condition is checked
                walkNode(until->predicate);
                walkNode(up_cast(until->statements));
            } else {
                // Regular until: condition is checked first, then body
                associateAssertionCommentsToNode(node);
                walkNode(until->predicate);
                walkNode(up_cast(until->statements));
            }
            consumeCommentsInsideNode(node, "until");
            break;
        }
        case PM_WHEN_NODE: {
            auto *when = down_cast<pm_when_node_t>(node);
            walkNode(up_cast(when->statements));
            if (when->statements) {
                consumeCommentsInsideNode(up_cast(when->statements), "when");
            }
            break;
        }
        case PM_WHILE_NODE: {
            auto *while_ = down_cast<pm_while_node_t>(node);
            // Check if this is post-condition while (modifier form: "body while condition")
            bool isModifier = while_->base.flags & PM_LOOP_FLAGS_BEGIN_MODIFIER;

            if (isModifier) {
                // Post while: body executes first, then condition is checked
                walkNode(while_->predicate);
                walkNode(up_cast(while_->statements));
            } else {
                // Regular while: condition is checked first, then body
                associateAssertionCommentsToNode(node);
                walkNode(while_->predicate);
                walkNode(up_cast(while_->statements));
            }
            consumeCommentsInsideNode(node, "while");
            break;
        }
        case PM_PROGRAM_NODE: {
            auto *program = down_cast<pm_program_node_t>(node);
            if (program->statements) {
                walkStatements(program->statements->body);
            }
            break;
        }
        case PM_STATEMENTS_NODE: {
            auto *statements = down_cast<pm_statements_node_t>(node);
            walkStatements(statements->body);
            break;
        }
        case PM_PARENTHESES_NODE: {
            auto *paren = down_cast<pm_parentheses_node_t>(node);
            associateAssertionCommentsToNode(node);
            walkNode(paren->body);
            consumeCommentsInsideNode(node, "begin");
            break;
        }
        case PM_LOCAL_VARIABLE_OPERATOR_WRITE_NODE: {
            walkAssignmentNode<pm_local_variable_operator_write_node_t>(node);
            break;
        }
        case PM_LOCAL_VARIABLE_AND_WRITE_NODE: {
            walkAssignmentNode<pm_local_variable_and_write_node_t>(node);
            break;
        }
        case PM_LOCAL_VARIABLE_OR_WRITE_NODE: {
            walkAssignmentNode<pm_local_variable_or_write_node_t>(node);
            break;
        }
        case PM_CONSTANT_OPERATOR_WRITE_NODE: {
            walkAssignmentNode<pm_constant_operator_write_node_t>(node);
            break;
        }
        case PM_CONSTANT_AND_WRITE_NODE: {
            walkAssignmentNode<pm_constant_and_write_node_t>(node);
            break;
        }
        case PM_CONSTANT_OR_WRITE_NODE: {
            walkAssignmentNode<pm_constant_or_write_node_t>(node);
            break;
        }
        case PM_INSTANCE_VARIABLE_OPERATOR_WRITE_NODE: {
            walkAssignmentNode<pm_instance_variable_operator_write_node_t>(node);
            break;
        }
        case PM_INSTANCE_VARIABLE_AND_WRITE_NODE: {
            walkAssignmentNode<pm_instance_variable_and_write_node_t>(node);
            break;
        }
        case PM_INSTANCE_VARIABLE_OR_WRITE_NODE: {
            walkAssignmentNode<pm_instance_variable_or_write_node_t>(node);
            break;
        }
        case PM_CLASS_VARIABLE_OPERATOR_WRITE_NODE: {
            walkAssignmentNode<pm_class_variable_operator_write_node_t>(node);
            break;
        }
        case PM_CLASS_VARIABLE_AND_WRITE_NODE: {
            walkAssignmentNode<pm_class_variable_and_write_node_t>(node);
            break;
        }
        case PM_CLASS_VARIABLE_OR_WRITE_NODE: {
            walkAssignmentNode<pm_class_variable_or_write_node_t>(node);
            break;
        }
        case PM_GLOBAL_VARIABLE_OPERATOR_WRITE_NODE: {
            walkAssignmentNode<pm_global_variable_operator_write_node_t>(node);
            break;
        }
        case PM_GLOBAL_VARIABLE_AND_WRITE_NODE: {
            walkAssignmentNode<pm_global_variable_and_write_node_t>(node);
            break;
        }
        case PM_GLOBAL_VARIABLE_OR_WRITE_NODE: {
            walkAssignmentNode<pm_global_variable_or_write_node_t>(node);
            break;
        }
        case PM_LOCAL_VARIABLE_WRITE_NODE: {
            walkAssignmentNode<pm_local_variable_write_node_t>(node);
            break;
        }
        case PM_CONSTANT_WRITE_NODE: {
            walkAssignmentNode<pm_constant_write_node_t>(node);
            break;
        }
        case PM_INSTANCE_VARIABLE_WRITE_NODE: {
            walkAssignmentNode<pm_instance_variable_write_node_t>(node);
            break;
        }
        case PM_CLASS_VARIABLE_WRITE_NODE: {
            walkAssignmentNode<pm_class_variable_write_node_t>(node);
            break;
        }
        case PM_GLOBAL_VARIABLE_WRITE_NODE: {
            walkAssignmentNode<pm_global_variable_write_node_t>(node);
            break;
        }
        default: {
            associateAssertionCommentsToNode(node);
            consumeCommentsInsideNode(node, "other");
            break;
        }
    }
}

CommentMapPrismNode CommentsAssociatorPrism::run(pm_node_t *node) {
    // Remove any comments that don't start with RBS prefixes
    for (auto it = commentByLine.begin(); it != commentByLine.end();) {
        if (!absl::StartsWith(it->second.string, RBS_PREFIX) &&
            !absl::StartsWith(it->second.string, ANNOTATION_PREFIX) &&
            !absl::StartsWith(it->second.string, MULTILINE_RBS_PREFIX)) {
            it = commentByLine.erase(it);
        } else {
            ++it;
        }
    }

    lastLine = 0;
    walkNode(node);

    // Check for any remaining comments
    for (const auto &[line, comment] : commentByLine) {
        if (absl::StartsWith(comment.string, RBS_PREFIX) || absl::StartsWith(comment.string, MULTILINE_RBS_PREFIX)) {
            if (auto e = ctx.beginError(comment.loc, core::errors::Rewriter::RBSUnusedComment)) {
                e.setHeader("Unused RBS comment. Couldn't associate it with a method definition or a type assertion");
            }
        }
    }

    return CommentMapPrismNode{signaturesForNode, assertionsForNode};
}

CommentsAssociatorPrism::CommentsAssociatorPrism(core::MutableContext ctx, parser::Prism::Parser &parser,
                                                 vector<core::LocOffsets> commentLocations)
    : ctx(ctx), parser(parser), prism(parser), commentLocations(commentLocations), commentByLine() {
    for (auto &loc : commentLocations) {
        auto commentString = ctx.file.data(ctx).source().substr(loc.beginPos(), loc.endPos() - loc.beginPos());
        auto start32 = static_cast<uint32_t>(loc.beginPos());
        auto end32 = static_cast<uint32_t>(loc.endPos());
        auto comment = CommentNodePrism{core::LocOffsets{start32, end32}, commentString};

        auto line = core::Loc::pos2Detail(ctx.file.data(ctx), start32).line;
        commentByLine[line] = comment;
    }
}

} // namespace sorbet::rbs
