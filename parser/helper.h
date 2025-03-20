#ifndef SORBET_PARSER_HELPER_H
#define SORBET_PARSER_HELPER_H

#include "parser/parser.h"

using namespace std;

namespace sorbet::parser {

class MK {
public:
    static unique_ptr<parser::Node> T(core::LocOffsets loc) {
        return make_unique<parser::Const>(loc, make_unique<parser::Cbase>(loc), core::Names::Constants::T());
    }

    static unique_ptr<parser::Node> TUntyped(core::LocOffsets loc) {
        return make_unique<parser::Const>(loc, T(loc), core::Names::Constants::Untyped());
    }
};

} // namespace sorbet::parser

#endif // SORBET_PARSER_HELPER_H
