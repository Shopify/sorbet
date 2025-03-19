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

core::LocOffsets locFromRange(core::LocOffsets loc, const range &range);

class Signature {
public:
    std::vector<Comment> comments;

    Signature(std::vector<Comment> comments) : comments(comments) {}

    core::LocOffsets loc() const {
        return {
            comments.front().loc.beginPos(),
            comments.back().loc.endPos(),
        };
    }

    std::string string() const {
        std::string result;
        for (const auto &comment : comments) {
            result += comment.string;
        }
        return result;
    }

    core::LocOffsets mapLocForRange(const range &range) const {
        if (comments.size() == 1) {
            return locFromRange(loc(), range);
        }

        unsigned int cursor = 0;
        for (const auto &comment : comments) {
            auto commentStartChar = cursor;
            auto commentEndChar = cursor + comment.loc.endPos() - comment.loc.beginPos() - 2;
            if (commentStartChar <= range.start.char_pos && commentEndChar >= range.start.char_pos) {
                auto offset = range.start.char_pos - commentStartChar + 3;
                return {
                    comment.loc.beginPos() + offset,
                    comment.loc.beginPos() + offset + range.end.char_pos - range.start.char_pos,
                };
            }
            cursor += commentEndChar;
        }
        return core::LocOffsets::none();
    }
};

} // namespace sorbet::rbs

#endif // SORBET_RBS_COMMON_H
