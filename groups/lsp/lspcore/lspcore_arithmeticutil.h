#ifndef INCLUDED_LSPCORE_ARITHMETICUTIL
#define INCLUDED_LSPCORE_ARITHMETICUTIL

#include <lspcore_nativeprocedureutil.h>

namespace lspcore {

struct ArithmeticUtil {
#define FUNCTION(NAME) void NAME(const NativeProcedureUtil::Arguments&)

    static FUNCTION(add);
    static FUNCTION(subtract);
    static FUNCTION(multiply);
    static FUNCTION(divide);

    static FUNCTION(equal);
    static FUNCTION(less);
    static FUNCTION(lessOrEqual);
    static FUNCTION(greater);
    static FUNCTION(greaterOrEqual);

#undef FUNCTION
};

}  // namespace lspcore

#endif
