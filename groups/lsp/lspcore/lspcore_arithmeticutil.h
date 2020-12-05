#ifndef INCLUDED_LSPCORE_ARITHMETICUTIL
#define INCLUDED_LSPCORE_ARITHMETICUTIL

#include <lspcore_nativeprocedureutil.h>

namespace lspcore {

struct ArithmeticUtil {
    static void add(bsl::vector<bdld::Datum>& argsAndOutput,
                    Environment&              environment,
                    bslma::Allocator*         allocator);
    static void subtract(bsl::vector<bdld::Datum>& argsAndOutput,
                         Environment&              environment,
                         bslma::Allocator*         allocator);
    static void multiply(bsl::vector<bdld::Datum>& argsAndOutput,
                         Environment&              environment,
                         bslma::Allocator*         allocator);
    static void divide(bsl::vector<bdld::Datum>& argsAndOutput,
                       Environment&              environment,
                       bslma::Allocator*         allocator);
    // TODO: "=" should be somewhere else
    static void equate(bsl::vector<bdld::Datum>& argsAndOutput,
                       Environment&              environment,
                       bslma::Allocator*         allocator);
};

}  // namespace lspcore

#endif
