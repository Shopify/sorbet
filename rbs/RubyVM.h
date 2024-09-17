#ifndef RUBYVM_H
#define RUBYVM_H

#include <memory>

namespace sorbet::rbs {

class RubyVM {
public:
    static void initialize();
    static void cleanup();
};

} // namespace sorbet::rbs

#endif // RUBYVM_H
