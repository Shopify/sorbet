#ifndef SORBET_RBS_COMMENTS_ASSOCIATOR_H
#define SORBET_RBS_COMMENTS_ASSOCIATOR_H

#include "common/common.h"
#include "parser/parser.h"
#include <memory>
#include <regex>
#include <string_view>

namespace sorbet::rbs {

struct CommentNode {
    core::LocOffsets loc;
    std::string_view string;
};

// Maps from parser tree nodes to their associated RBS comments.
//
// All `parser::Node *` keys are non-owning pointers into the parser tree passed to
// CommentsAssociator::run(). They remain valid as long as the parser tree is alive.
// The pipeline holds the tree via `unique_ptr<parser::Node>` and passes it through
// SigsRewriter and AssertionsRewriter before the tree is consumed by the desugarer.
struct CommentMap {
    // A member of a Data.define call, optionally annotated with an inline `#:` type.
    // The `typeComment` field, when present, contains a CommentNode whose `string`
    // is a string_view into the file source buffer (owned by GlobalState). It remains
    // valid for the lifetime of the file data.
    struct DataDefineMember {
        core::NameRef name;
        core::LocOffsets nameLoc;
        std::optional<CommentNode> typeComment;
    };

    std::map<parser::Node *, std::vector<CommentNode>> signaturesForNode;
    std::map<parser::Node *, std::vector<CommentNode>> assertionsForNode;
    // Maps Data.define Send nodes to their members with inline type annotations.
    // Populated only when at least one symbol argument has a `#:` type comment.
    std::map<parser::Node *, std::vector<DataDefineMember>> dataDefineMembersForNode;
};

class CommentsAssociator {
public:
    static const std::string_view RBS_PREFIX;

    CommentsAssociator(core::MutableContext ctx, std::vector<core::LocOffsets> commentLocations);
    CommentMap run(std::unique_ptr<parser::Node> &tree);

private:
    static const std::string_view ANNOTATION_PREFIX;
    static const std::string_view MULTILINE_RBS_PREFIX;
    static const std::string_view BIND_PREFIX;

    core::MutableContext ctx;
    std::vector<core::LocOffsets> commentLocations;
    std::map<int, CommentNode> commentByLine;
    std::map<parser::Node *, std::vector<CommentNode>> signaturesForNode;
    std::map<parser::Node *, std::vector<CommentNode>> assertionsForNode;
    std::map<parser::Node *, std::vector<CommentMap::DataDefineMember>> dataDefineMembersForNode;
    std::vector<std::pair<bool, core::LocOffsets>> contextAllowingTypeAlias;
    int lastLine;

    bool isDataDefineSend(parser::Send *send);
    void associateDataDefineMemberTypes(parser::Send *send);
    void walkNode(parser::Node *node);
    void walkNodes(parser::NodeVec &nodes);
    void walkStatements(parser::NodeVec &nodes);
    std::unique_ptr<parser::Node> walkBody(parser::Node *node, std::unique_ptr<parser::Node> body);
    void processTrailingComments(parser::Node *node, parser::NodeVec &nodes);
    void associateAssertionCommentsToNode(parser::Node *node, bool adjustLocForHeredoc);
    void associateSignatureCommentsToNode(parser::Node *node);
    void consumeCommentsInsideNode(parser::Node *node, std::string kind);
    void consumeCommentsBetweenLines(int startLine, int endLine, std::string kind);
    void consumeCommentsUntilLine(int line);
    std::optional<uint32_t> locateTargetLine(parser::Node *node);

    int maybeInsertStandalonePlaceholders(parser::NodeVec &nodes, int index, int lastLine, int currentLine);
    bool typeAliasAllowedInContext();
};

} // namespace sorbet::rbs
#endif // SORBET_RBS_COMMENTS_ASSOCIATOR_H
