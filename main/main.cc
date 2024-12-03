#include "common/common.h"
#include "main/options/options.h"
#include "main/realmain.h"
#include "rbs/rbs_common.h"

int main(int argc, char *argv[]) {
    int result;
    try {
        result = sorbet::realmain::realmain(argc, argv);
    } catch (sorbet::EarlyReturnWithCode &c) {
        result = c.returnCode;
    }

    return result;
}
