#include "common/common.h"
#include "main/options/options.h"
#include "main/realmain.h"
#include "rbs/rbs_common.h"
#include "rbs/RubyVM.h"

int main(int argc, char *argv[]) {
    sorbet::rbs::RubyVM::initialize();

    int result;
    try {
        result = sorbet::realmain::realmain(argc, argv);
    } catch (sorbet::EarlyReturnWithCode &c) {
        result = c.returnCode;
    }

    sorbet::rbs::RubyVM::cleanup();
    return result;
}
