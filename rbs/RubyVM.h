#ifndef RUBYVM_H
#define RUBYVM_H

#include <memory>

namespace sorbet::rbs {

class RubyVM {
public:
    static void initialize();
    static void cleanup();
private:
    static void handle_ruby_exception(const char* context, int state);
};

} // namespace sorbet::rbs

#endif // RUBYVM_H
