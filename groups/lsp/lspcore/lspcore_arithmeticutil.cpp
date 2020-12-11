#include <bdlb_arrayutil.h>
#include <bdld_datummaker.h>
#include <bdldfp_decimal.h>
#include <bsl_algorithm.h>
#include <bsl_functional.h>
#include <bsl_limits.h>
#include <bsl_numeric.h>
#include <bsl_utility.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>
#include <bsls_types.h>
#include <lspcore_arithmeticutil.h>

using namespace BloombergLP;

namespace lspcore {
namespace {

template <typename RESULT>
RESULT the(const bdld::Datum&);

template <>
bdldfp::Decimal64 the<bdldfp::Decimal64>(const bdld::Datum& datum) {
    return datum.theDecimal64();
}

template <>
bsls::Types::Int64 the<bsls::Types::Int64>(const bdld::Datum& datum) {
    return datum.theInteger64();
}

template <>
int the<int>(const bdld::Datum& datum) {
    return datum.theInteger();
}

template <>
double the<double>(const bdld::Datum& datum) {
    return datum.theDouble();
}

template <template <typename> class OPERATOR, typename NUMBER>
struct Operator {
    NUMBER operator()(NUMBER soFar, const bdld::Datum& number) const {
        return OPERATOR<NUMBER>()(soFar, the<NUMBER>(number));
    }
};

struct Classification {
    enum Kind {
        e_COMMON_TYPE,
        e_SAME_TYPE,
        e_ERROR_INCOMPATIBLE_NUMBER_TYPES,
        e_ERROR_NON_NUMERIC_TYPE
    };

    Kind                  kind;
    bdld::Datum::DataType type;
};

typedef bsls::Types::Uint64 Bits;

// There must be no more than 64 types of 'bdld::Datum', otherwise we wouldn't
// be able to use the type 'Bits' as a bitmask. In some far infinite future, we
// could switch to 'std::bitset' if necessary.
BSLMF_ASSERT(bdld::Datum::k_NUM_TYPES <= bsl::numeric_limits<Bits>::digits);

Classification classify(Bits types) {
    // cases:
    //
    // - any non-number types → e_ERROR_NON_NUMERIC_TYPE
    // - incompatible type combinations → e_ERROR_INCOMPATIBLE_NUMBER_TYPES
    // - all the same → e_SAME_TYPE
    // - otherwise, pick out the common type → e_COMMON_TYPE

#define TYPE(NAME) (Bits(1) << bdld::Datum::e_##NAME)

    const Bits numbers =
        TYPE(INTEGER) | TYPE(INTEGER64) | TYPE(DOUBLE) | TYPE(DECIMAL64);

    const Bits invalidCombos[] = { TYPE(INTEGER64) | TYPE(DOUBLE),
                                   TYPE(INTEGER64) | TYPE(DECIMAL64),
                                   TYPE(DOUBLE) | TYPE(DECIMAL64) };

    // check for non-number types
    if (types & ~numbers) {
        Classification result;
        result.kind = Classification::e_ERROR_NON_NUMERIC_TYPE;
        return result;
    }

    // check for incompatible combinations of number types
    for (bsl::size_t i = 0; i < bdlb::ArrayUtil::size(invalidCombos); ++i) {
        if ((types & invalidCombos[i]) == invalidCombos[i]) {
            Classification result;
            result.kind = Classification::e_ERROR_INCOMPATIBLE_NUMBER_TYPES;
            return result;
        }
    }

    // check for all-the-same-number-type
    Classification result;
    switch (types) {
        case TYPE(INTEGER):
            result.kind = Classification::e_SAME_TYPE;
            result.type = bdld::Datum::e_INTEGER;
            return result;
        case TYPE(INTEGER64):
            result.kind = Classification::e_SAME_TYPE;
            result.type = bdld::Datum::e_INTEGER64;
            return result;
        case TYPE(DOUBLE):
            result.kind = Classification::e_SAME_TYPE;
            result.type = bdld::Datum::e_DOUBLE;
            return result;
        case TYPE(DECIMAL64):
            result.kind = Classification::e_SAME_TYPE;
            result.type = bdld::Datum::e_DECIMAL64;
            return result;
        default:
            break;
    }

    // At this point, we have exactly two number types, and one of them is int.
    // The common type is just the other type.
    result.kind = Classification::e_COMMON_TYPE;
    switch (const Bits other = types & ~TYPE(INTEGER)) {
        case TYPE(INTEGER64):
            result.type = bdld::Datum::e_INTEGER64;
            return result;
        case TYPE(DOUBLE):
            result.type = bdld::Datum::e_DOUBLE;
            return result;
        default:
            (void)other;
            BSLS_ASSERT(other == TYPE(DECIMAL64));
            result.type = bdld::Datum::e_DECIMAL64;
            return result;
    }
}

Classification classify(const bsl::vector<bdld::Datum>& data) {
    Bits types = 0;
    for (bsl::size_t i = 0; i < data.size(); ++i) {
        types |= Bits(1) << data[i].type();
    }
    return classify(types);
}

template <typename NUMBER>
void homogeneousAdd(bsl::vector<bdld::Datum>& argsAndOutput,
                    bslma::Allocator*         allocator) {
    const NUMBER result = bsl::accumulate(argsAndOutput.begin(),
                                          argsAndOutput.end(),
                                          NUMBER(0),
                                          Operator<bsl::plus, NUMBER>());

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::DatumMaker(allocator)(result);
}

template <typename NUMBER>
void homogeneousSubtract(bsl::vector<bdld::Datum>& argsAndOutput,
                         bslma::Allocator*         allocator) {
    BSLS_ASSERT(!argsAndOutput.empty());

    NUMBER result;
    if (argsAndOutput.size() == 1) {
        result = -the<NUMBER>(argsAndOutput[0]);
    }
    else {
        result = bsl::accumulate(argsAndOutput.begin() + 1,
                                 argsAndOutput.end(),
                                 the<NUMBER>(argsAndOutput[0]),
                                 Operator<bsl::minus, NUMBER>());
    }

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::DatumMaker(allocator)(result);
}

template <typename NUMBER>
void homogeneousMultiply(bsl::vector<bdld::Datum>& argsAndOutput,
                         bslma::Allocator*         allocator) {
    const NUMBER result = bsl::accumulate(argsAndOutput.begin(),
                                          argsAndOutput.end(),
                                          NUMBER(1),
                                          Operator<bsl::multiplies, NUMBER>());

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::DatumMaker(allocator)(result);
}

template <typename NUMBER>
void homogeneousDivide(bsl::vector<bdld::Datum>& argsAndOutput,
                       bslma::Allocator*         allocator) {
    BSLS_ASSERT(!argsAndOutput.empty());

    NUMBER result;
    if (argsAndOutput.size() == 1) {
        result = the<NUMBER>(argsAndOutput[0]);
    }
    else {
        result = bsl::accumulate(argsAndOutput.begin() + 1,
                                 argsAndOutput.end(),
                                 the<NUMBER>(argsAndOutput[0]),
                                 Operator<bsl::divides, NUMBER>());
    }

    argsAndOutput.resize(1);
    argsAndOutput[0] = bdld::DatumMaker(allocator)(result);
}

template <typename OUTPUT>
class NumericConvert {
    bdld::DatumMaker d_make;

  public:
    explicit NumericConvert(bslma::Allocator* allocator)
    : d_make(allocator) {
    }

    bdld::Datum operator()(const bdld::Datum& datum) const {
        if (datum.type() == bdld::Datum::e_INTEGER) {
            OUTPUT number(datum.theInteger());
            return d_make(number);
        }

        return datum;
    }
};

// TODO: document
bdld::Datum::DataType normalizeTypes(bsl::vector<bdld::Datum>& data,
                                     bslma::Allocator*         allocator) {
    Classification result = classify(data);

    switch (result.kind) {
        case Classification::e_COMMON_TYPE:
            switch (result.type) {
                case bdld::Datum::e_INTEGER64:
                    bsl::transform(
                        data.begin(),
                        data.end(),
                        data.begin(),
                        NumericConvert<bsls::Types::Int64>(allocator));
                    break;
                case bdld::Datum::e_DOUBLE:
                    bsl::transform(data.begin(),
                                   data.end(),
                                   data.begin(),
                                   NumericConvert<double>(allocator));
                    break;
                default:
                    BSLS_ASSERT(result.type == bdld::Datum::e_DECIMAL64);
                    bsl::transform(
                        data.begin(),
                        data.end(),
                        data.begin(),
                        NumericConvert<bdldfp::Decimal64>(allocator));
            }
        // fall through
        case Classification::e_SAME_TYPE:
            return result.type;
        case Classification::e_ERROR_INCOMPATIBLE_NUMBER_TYPES:
            throw bdld::Datum::createError(
                -1,
                "incompatible numeric types passed to an arithmetic procedure",
                allocator);
        default:
            BSLS_ASSERT(result.kind ==
                        Classification::e_ERROR_NON_NUMERIC_TYPE);
            throw bdld::Datum::createError(
                -1,
                "non-numeric type passed to an arithmetic procedure",
                allocator);
    }
}

}  // namespace

#define DISPATCH(FUNCTION)                                                 \
    switch (bdld::Datum::DataType type =                                   \
                normalizeTypes(argsAndOutput, allocator)) {                \
        case bdld::Datum::e_INTEGER:                                       \
            return FUNCTION<int>(argsAndOutput, allocator);                \
        case bdld::Datum::e_INTEGER64:                                     \
            return FUNCTION<bsls::Types::Int64>(argsAndOutput, allocator); \
        case bdld::Datum::e_DOUBLE:                                        \
            return FUNCTION<double>(argsAndOutput, allocator);             \
        default:                                                           \
            (void)type;                                                    \
            BSLS_ASSERT(type == bdld::Datum::e_DECIMAL64);                 \
            return FUNCTION<bdldfp::Decimal64>(argsAndOutput, allocator);  \
    }

void ArithmeticUtil::add(bsl::vector<bdld::Datum>& argsAndOutput,
                         Environment&,
                         bslma::Allocator* allocator) {
    DISPATCH(homogeneousAdd)
}

void ArithmeticUtil::subtract(bsl::vector<bdld::Datum>& argsAndOutput,
                              Environment&,
                              bslma::Allocator* allocator) {
    if (argsAndOutput.empty()) {
        throw bdld::Datum::createError(
            -1, "substraction requires at least one operand", allocator);
    }

    DISPATCH(homogeneousSubtract)
}

void ArithmeticUtil::multiply(bsl::vector<bdld::Datum>& argsAndOutput,
                              Environment&,
                              bslma::Allocator* allocator) {
    DISPATCH(homogeneousMultiply)
}

void ArithmeticUtil::divide(bsl::vector<bdld::Datum>& argsAndOutput,
                            Environment&,
                            bslma::Allocator* allocator) {
    if (argsAndOutput.empty()) {
        throw bdld::Datum::createError(
            -1, "division requires at least one operand", allocator);
    }

    DISPATCH(homogeneousDivide)
}

#undef DISPATCH

void ArithmeticUtil::equal(bsl::vector<bdld::Datum>& argsAndOutput,
                           Environment&,
                           bslma::Allocator* allocator) {
    if (argsAndOutput.empty()) {
        throw bdld::Datum::createError(
            -1, "equlity comparison requires at least one operand", allocator);
    }
    if (classify(argsAndOutput).kind ==
        Classification::e_ERROR_NON_NUMERIC_TYPE) {
        throw bdld::Datum::createError(
            -1, "equlity comparison requires all numeric operands", allocator);
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
