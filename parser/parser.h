#ifndef SORBET_PARSER_PARSER_H
#define SORBET_PARSER_PARSER_H

#include "Node.h"

namespace sorbet::parser {

class Parser final {
public:
    struct Settings {
        bool traceLexer : 1;
        bool traceParser : 1;
        bool indentationAware : 1;

        Settings() : traceLexer(false), traceParser(false), indentationAware(false) {}
        Settings(bool traceLexer, bool traceParser, bool indentationAware)
            : traceLexer(traceLexer), traceParser(traceParser), indentationAware(indentationAware) {}

        Settings withIndentationAware() {
            return Settings{this->traceLexer, this->traceParser, true};
        }
    };

    // struct ParseResult {
    //     std::unique_ptr<Node> tree;
    //     std::vector<size_t> comments;
    // };

    static std::unique_ptr<Node> run(core::GlobalState &gs, core::FileRef file, Settings settings,
                                     std::vector<std::string> initialLocals = {});

    //     static const std::vector<size_t> &get_comment_locations() {
    //         return last_driver->lex.comment_locations;
    //     }

    // private:
    //     static ruby_parser::base_driver *last_driver;
};

} // namespace sorbet::parser

#endif // SORBET_PARSER_PARSER_H
