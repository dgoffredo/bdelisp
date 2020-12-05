#include <bdldfp_decimal.h>
#include <lspcore_arithmeticutil.h>

using namespace BloombergLP;

namespace lspcore {

// TODO: For now all of these functions require 'Decimal64' arguments.
// I'll generalize it later.

void ArithmeticUtil::add(bsl::vector<bdld::Datum>& argsAndOutput,
                         Environment&,
                         bslma::Allocator* allocator) {
    bdldfp::Decimal64 result(0);

    bsl::vector<bdld::Datum>::const_iterator iter = argsAndOutput.begin();
    for (; iter != argsAndOutput.end(); ++iter) {
        if (!iter->isDecimal64()) {
            throw bdld::Datum::createError(
                -1, "only Decimal64 is supported for arithmetic", allocator);
        }
        result += iter->theDecimal64();
    }

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::Datum::createDecimal64(result, allocator);
}

void ArithmeticUtil::subtract(bsl::vector<bdld::Datum>& argsAndOutput,
                              Environment&,
                              bslma::Allocator* allocator) {
    (void)argsAndOutput;
    throw bdld::Datum::createError(-1, "not implemented", allocator);
}

void ArithmeticUtil::multiply(bsl::vector<bdld::Datum>& argsAndOutput,
                              Environment&,
                              bslma::Allocator* allocator) {
    (void)argsAndOutput;
    throw bdld::Datum::createError(-1, "not implemented", allocator);
}

void ArithmeticUtil::divide(bsl::vector<bdld::Datum>& argsAndOutput,
                            Environment&,
                            bslma::Allocator* allocator) {
    (void)argsAndOutput;
    throw bdld::Datum::createError(-1, "not implemented", allocator);
}

void ArithmeticUtil::equate(bsl::vector<bdld::Datum>& argsAndOutput,
                            Environment&,
                            bslma::Allocator* allocator) {
    (void)argsAndOutput;
    throw bdld::Datum::createError(-1, "not implemented", allocator);
}

}  // namespace lspcore
