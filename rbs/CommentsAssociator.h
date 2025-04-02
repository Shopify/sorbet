#ifndef SORBET_RBS_COMMENTS_ASSOCIATOR_H
#define SORBET_RBS_COMMENTS_ASSOCIATOR_H

#include "parser/parser.h"
#include <memory>

namespace sorbet::rbs {

class CommentsAssociator {
public:
    CommentsAssociator(core::MutableContext ctx, std::vector<std::pair<size_t, size_t>> commentLocations)
        : ctx(ctx), commentLocations(commentLocations){};
    void run(std::unique_ptr<parser::Node> tree);

private:
    core::MutableContext ctx;
    std::vector<std::pair<size_t, size_t>> commentLocations;
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_COMMENTS_ASSOCIATOR_H
