#ifndef INCLUDED_LSPCORE_BUILTINPROCEDURES
#define INCLUDED_LSPCORE_BUILTINPROCEDURES

#include <lspcore_nativeprocedureutil.h>

namespace lspcore {

struct BuiltinProcedures {
#define FUNCTION(NAME) void NAME(const NativeProcedureUtil::Arguments&)

    static FUNCTION(isPair);
    static FUNCTION(pair);
    static FUNCTION(pairFirst);
    static FUNCTION(pairSecond);
    static FUNCTION(isNull);
    static FUNCTION(equal);

#undef FUNCTION
};

}  // namespace lspcore

#endif
