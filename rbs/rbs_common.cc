#include "rbs/rbs_common.h"

namespace sorbet::rbs {

core::LocOffsets Signature::mapLocForRange(const range &range) const {
    // Get the first comment and initialize tracking variables
    auto firstComment = comments.front();
    auto commentStartLocOffset = 2; // Skip the #: or #| prefix

    // Calculate start positions for the first comment
    uint32_t commentBeginLoc = firstComment.loc.beginPos() + commentStartLocOffset;
    uint32_t commentEndLoc = firstComment.loc.endPos();

    // Calculate absolute position of the range start within the document
    uint32_t rangeBeginLoc = commentBeginLoc + range.start.char_pos;
    uint32_t rangeEndLoc = rangeBeginLoc + (range.end.char_pos - range.start.char_pos);

    // Check if the range is within the first comment
    if (rangeBeginLoc >= commentBeginLoc && rangeEndLoc <= commentEndLoc) {
        return core::LocOffsets{rangeBeginLoc, rangeEndLoc};
    }

    // We need to find which comment contains our range
    // The signature string() concatenates all comments excluding prefixes
    uint32_t currentPos = firstComment.string.size() - 2; // Skip "#:" from first comment

    for (size_t i = 1; i < comments.size(); i++) {
        const auto &comment = comments[i];
        uint32_t commentContentLength = comment.string.size() - 2; // Skip "#|" from comment

        // If range starts in this comment
        if (range.start.char_pos < currentPos + commentContentLength) {
            // Calculate how far into this comment the range starts
            uint32_t offsetInComment = range.start.char_pos - currentPos;

            // Map to absolute document position
            uint32_t absoluteBeginPos = comment.loc.beginPos() + commentStartLocOffset + offsetInComment;
            uint32_t absoluteEndPos = absoluteBeginPos + (range.end.char_pos - range.start.char_pos);

            return core::LocOffsets{absoluteBeginPos, absoluteEndPos};
        }

        // Move to next comment
        currentPos += commentContentLength;
    }

    // Couldn't map the range
    return core::LocOffsets::none();
}

} // namespace sorbet::rbs
