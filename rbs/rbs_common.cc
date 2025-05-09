#include "rbs/rbs_common.h"

namespace sorbet::rbs {

core::LocOffsets RBSDeclaration::commentLoc() const {
    return comments.front().commentLoc.join(comments.back().commentLoc);
}

core::LocOffsets RBSDeclaration::firstLineTypeLoc() const {
    return comments.front().typeLoc;
}

core::LocOffsets RBSDeclaration::fullTypeLoc() const {
    return comments.front().typeLoc.join(comments.back().typeLoc);
}

core::LocOffsets RBSDeclaration::typeLocFromRange(const rbs_range_t &range) const {
    int rangeOffset = range.start.char_pos;
    int rangeLength = range.end.char_pos - range.start.char_pos;

    for (const auto &comment : comments) {
        int commentTypeLength = comment.typeLoc.endLoc - comment.typeLoc.beginLoc;
        if (rangeOffset < commentTypeLength) {
            auto beginLoc = comment.typeLoc.beginLoc + rangeOffset;
            auto endLoc = beginLoc + rangeLength;

            if (rangeLength > commentTypeLength) {
                endLoc = comment.typeLoc.endLoc;
            }

            return core::LocOffsets{beginLoc, endLoc};
        }
        rangeOffset -= commentTypeLength;
    }
    return comments.front().typeLoc;
}

bool isVisibilitySend(const parser::Send *send) {
    return send->receiver == nullptr && send->args.size() == 1 &&
           (parser::isa_node<parser::DefMethod>(send->args[0].get()) ||
            parser::isa_node<parser::DefS>(send->args[0].get())) &&
           (send->method == core::Names::private_() || send->method == core::Names::protected_() ||
            send->method == core::Names::public_() || send->method == core::Names::privateClassMethod() ||
            send->method == core::Names::publicClassMethod() || send->method == core::Names::packagePrivate() ||
            send->method == core::Names::packagePrivateClassMethod());
}

bool isAttrAccessorSend(const parser::Send *send) {
    return (send->receiver == nullptr || parser::isa_node<parser::Self>(send->receiver.get())) &&
           (send->method == core::Names::attrReader() || send->method == core::Names::attrWriter() ||
            send->method == core::Names::attrAccessor());
}

} // namespace sorbet::rbs
