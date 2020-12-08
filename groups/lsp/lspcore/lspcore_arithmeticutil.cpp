#include <bdldfp_decimal.h>
#include <bsl_algorithm.h>
#include <bsl_functional.h>
#include <bsl_numeric.h>
#include <lspcore_arithmeticutil.h>

using namespace BloombergLP;

namespace lspcore {

// TODO: For now all of these functions require 'Decimal64' arguments.
// I'll generalize it later.

template <template <typename> class OPERATOR>
class Operator {
    bslma::Allocator* d_allocator_p;

  public:
    explicit Operator(bslma::Allocator* allocator)
    : d_allocator_p(allocator) {
    }

    bdldfp::Decimal64 operator()(bdldfp::Decimal64  soFar,
                                 const bdld::Datum& number) const {
        if (!number.isDecimal64()) {
            throw bdld::Datum::createError(
                -1,
                "only Decimal64 is supported for arithmetic",
                d_allocator_p);
        }

        return OPERATOR<bdldfp::Decimal64>()(soFar, number.theDecimal64());
    }
};

void ArithmeticUtil::add(bsl::vector<bdld::Datum>& argsAndOutput,
                         Environment&,
                         bslma::Allocator* allocator) {
    const bdldfp::Decimal64 result =
        bsl::accumulate(argsAndOutput.begin(),
                        argsAndOutput.end(),
                        bdldfp::Decimal64(0),
                        Operator<bsl::plus>(allocator));

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::Datum::createDecimal64(result, allocator);
}

void ArithmeticUtil::subtract(bsl::vector<bdld::Datum>& argsAndOutput,
                              Environment&,
                              bslma::Allocator* allocator) {
    if (argsAndOutput.empty()) {
        throw bdld::Datum::createError(
            -1, "substraction requires at least one operand", allocator);
    }
    if (!argsAndOutput[0].isDecimal64()) {
        throw bdld::Datum::createError(
            -1, "only Decimal64 is supported for arithmetic", allocator);
    }

    bdldfp::Decimal64 result;
    if (argsAndOutput.size() == 1) {
        result = -argsAndOutput[0].theDecimal64();
    }
    else {
        result = bsl::accumulate(argsAndOutput.begin() + 1,
                                 argsAndOutput.end(),
                                 argsAndOutput[0].theDecimal64(),
                                 Operator<bsl::minus>(allocator));
    }

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::Datum::createDecimal64(result, allocator);
}

void ArithmeticUtil::multiply(bsl::vector<bdld::Datum>& argsAndOutput,
                              Environment&,
                              bslma::Allocator* allocator) {
    const bdldfp::Decimal64 result =
        bsl::accumulate(argsAndOutput.begin(),
                        argsAndOutput.end(),
                        bdldfp::Decimal64(1),
                        Operator<bsl::multiplies>(allocator));

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::Datum::createDecimal64(result, allocator);
}

void ArithmeticUtil::divide(bsl::vector<bdld::Datum>& argsAndOutput,
                            Environment&,
                            bslma::Allocator* allocator) {
    if (argsAndOutput.empty()) {
        throw bdld::Datum::createError(
            -1, "division requires at least one operand", allocator);
    }
    if (!argsAndOutput[0].isDecimal64()) {
        throw bdld::Datum::createError(
            -1, "only Decimal64 is supported for arithmetic", allocator);
    }

    bdldfp::Decimal64 result;
    if (argsAndOutput.size() == 1) {
        result = -argsAndOutput[0].theDecimal64();
    }
    else {
        result = bsl::accumulate(argsAndOutput.begin() + 1,
                                 argsAndOutput.end(),
                                 argsAndOutput[0].theDecimal64(),
                                 Operator<bsl::divides>(allocator));
    }

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::Datum::createDecimal64(result, allocator);
}

void ArithmeticUtil::equate(bsl::vector<bdld::Datum>& argsAndOutput,
                            Environment&,
                            bslma::Allocator* allocator) {
    if (argsAndOutput.empty()) {
        throw bdld::Datum::createError(
            -1, "equlity comparison requires at least one operand", allocator);
    }
    if (!argsAndOutput[0].isDecimal64()) {
        throw bdld::Datum::createError(
            -1, "only Decimal64 is supported for arithmetic", allocator);
    }

    bool result;
    if (argsAndOutput.size() == 1) {
        result = true;
    }
    else {
        result = std::adjacent_find(argsAndOutput.begin(),
                                    argsAndOutput.end(),
                                    bsl::not_equal_to<bdld::Datum>()) ==
                 argsAndOutput.end();
    }

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::Datum::createBoolean(result);
}

}  // namespace lspcore
