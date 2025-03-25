#ifndef SORBET_RBS_COMMON_H
#define SORBET_RBS_COMMON_H

extern "C" {
#include "include/rbs.h"
}

#include "core/LocOffsets.h"
#include "rbs/Parser.h"

namespace sorbet::rbs {

/**
 * A single RBS type comment found on a method definition or accessor.
 *
 * RBS type comments are formatted as `#: () -> void` for methods or `#: Integer` for attributes.
 */
struct Comment {
    core::LocOffsets loc; // Full location of the multi-line comment (including #: and #|)
    std::string string;   // Concatenated single-line signature
};

class Signature {
public:
    std::vector<Comment> comments;

    Signature(std::vector<Comment> comments) : comments(comments) {}

    core::LocOffsets loc() const {
        return {
            comments.front().loc.beginPos() + 2,
            comments.back().loc.endPos(),
        };
    }

    std::string string() const {
        std::string result;
        for (const auto &comment : comments) {
            result += comment.string.substr(2);
        }
        return result;
    }

    core::LocOffsets locFromRange(const range &range) const;
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_COMMON_H
