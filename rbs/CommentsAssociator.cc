#include "rbs/CommentsAssociator.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "common/typecase.h"
#include "core/errors/rewriter.h"
#include "parser/helper.h"
#include "rbs/SignatureTranslator.h"

using namespace std;

namespace sorbet::rbs {

namespace {
struct CommentNode {
    core::LocOffsets loc;
    std::string_view string;
};

}; // namespace

void CommentsAssociator::run(unique_ptr<parser::Node> node) {
    std::vector<CommentNode> commentNodes;

    for (auto &[start, end] : commentLocations) {
        auto comment_string = ctx.file.data(ctx).source().substr(start, end - start);
        auto start32 = static_cast<uint32_t>(start);
        auto end32 = static_cast<uint32_t>(end);
        auto comment = CommentNode{core::LocOffsets{start32, end32}, comment_string};

        commentNodes.emplace_back(comment);
    }

    std::cout << "Comment nodes: " << commentNodes.size() << std::endl;
}

} // namespace sorbet::rbs
