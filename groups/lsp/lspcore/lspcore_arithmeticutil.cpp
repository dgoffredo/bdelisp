#include <bdlb_arrayutil.h>
#include <bdlb_bitutil.h>
#include <bdld_datummaker.h>
#include <bdldfp_decimal.h>
#include <bdldfp_decimalutil.h>
#include <bsl_algorithm.h>
#include <bsl_cmath.h>
#include <bsl_functional.h>
#include <bsl_limits.h>
#include <bsl_numeric.h>
#include <bsl_utility.h>
#include <bslmf_assert.h>
#include <bsls_assert.h>
#include <bsls_types.h>
#include <lspcore_arithmeticutil.h>
#include <math.h>  // isnan

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

int divideFives(bsl::uint64_t* ptr) {
    BSLS_ASSERT(ptr);
    bsl::uint64_t& value = *ptr;
    BSLS_ASSERT(value != 0);

    int count;
    for (count = 0; value % 5 == 0; ++count) {
        value /= 5;
    }

    return count;
}

bool notEqual(double binary, bdldfp::Decimal64 decimal) {
    // Ladies and gentlemen, I present to you the least efficient equality
    // predicate ever devised.
    //
    // The plan is to divide out the factors of 2 and 5 from the significands
    // of 'binary' and 'decimal'. Since the radix of 'binary' is 2 and the
    // radix of 'decimal' is 10 (which is 2*5), we can then compare the reduced
    // significands and the exponents.
    //
    // One optimization is possible for "find the factors of two in the
    // significand." For nonzero values, we can count the number of trailing
    // zero bits (that's the number of factors of two) and then shift right
    // that many bits.
    //
    // Factors of five we'll have to do the hard way, but we need check the
    // powers of five only if the powers of two happen to be equal.

    // 'bsl::uint64_t' and 'bsls::Types::Uint64' can be different types, and
    // the compiler will complain about aliasing. Here I just pick one and
    // copy to the other where necessary.
    typedef bsl::uint64_t Uint64;

    bsls::Types::Uint64 decimalMantissaArg;  // long vs. long long ugh...
    int                 decimalSign;         // -1 or +1
    int                 decimalExponent;
    switch (
        const int kind = bdldfp::DecimalUtil::decompose(
            &decimalSign, &decimalMantissaArg, &decimalExponent, decimal)) {
        case FP_NAN:
            return true;
        case FP_INFINITE:
            return binary !=
                   bsl::numeric_limits<double>::infinity() * decimalSign;
        case FP_SUBNORMAL:
            // same handling as in the 'FP_NORMAL' case ('default', below)
            break;
        case FP_ZERO:
            return binary != 0;
        default:
            (void)kind;
            BSLS_ASSERT(kind == FP_NORMAL);
    }

    Uint64 decimalMantissa = decimalMantissaArg;  // long vs. long long ugh...

    // Skip the calculations below if the values have different signs.
    const int binarySign = binary < 0 ? -1 : 1;
    if (decimalSign != binarySign) {
        return true;
    }

    // To decompose the binary floating point value, we use the standard
    // library function 'frexp' followed by a multiplication to guarantee an
    // integer-valued significand. This multiplication will not lose precision.
    int          binaryExponent;
    const double binaryNormalizedSignificand =
        std::frexp(binary, &binaryExponent);
    if (binaryNormalizedSignificand == 0 ||
        isnan(binaryNormalizedSignificand) ||
        binaryNormalizedSignificand ==
            std::numeric_limits<double>::infinity() ||
        binaryNormalizedSignificand ==
            -std::numeric_limits<double>::infinity()) {
        return true;
    }

    const int precision = 53;
    Uint64    binaryMantissa =
        binaryNormalizedSignificand * (Uint64(1) << (precision - 1));
    binaryExponent -= precision - 1;

    BSLS_ASSERT(binaryMantissa != 0);
    BSLS_ASSERT(decimalMantissa != 0);

    const int binaryUnset =
        bdlb::BitUtil::numTrailingUnsetBits(binaryMantissa);
    const int binaryTwos = binaryExponent + binaryUnset;

    const int decimalUnset =
        bdlb::BitUtil::numTrailingUnsetBits(decimalMantissa);
    const int decimalTwos = decimalExponent + decimalUnset;

    if (binaryTwos != decimalTwos) {
        return true;
    }

    binaryMantissa >>= binaryUnset;
    decimalMantissa >>= decimalUnset;

    const int binaryFives  = divideFives(&binaryMantissa);
    const int decimalFives = decimalExponent + divideFives(&decimalMantissa);

    return binaryFives != decimalFives || binaryMantissa != decimalMantissa;
}

class NotEqual {
    bslma::Allocator* d_allocator_p;

  public:
    explicit NotEqual(bslma::Allocator* allocator)
    : d_allocator_p(allocator) {
    }

    bool operator()(const bdld::Datum& left, const bdld::Datum& right) const {
        if (left.type() == right.type()) {
            return left != right;
        }

#define TYPE_PAIR(LEFT, RIGHT) \
    ((bsl::uint64_t(LEFT) << 32) | bsl::uint64_t(RIGHT))

        switch (TYPE_PAIR(left.type(), right.type())) {
            case TYPE_PAIR(bdld::Datum::e_INTEGER, bdld::Datum::e_INTEGER64):
                return left.theInteger() != right.theInteger64();
            case TYPE_PAIR(bdld::Datum::e_INTEGER64, bdld::Datum::e_INTEGER):
                return right.theInteger() != left.theInteger64();
            case TYPE_PAIR(bdld::Datum::e_INTEGER, bdld::Datum::e_DOUBLE):
                return left.theInteger() != right.theDouble();
            case TYPE_PAIR(bdld::Datum::e_DOUBLE, bdld::Datum::e_INTEGER):
                return right.theInteger() != left.theDouble();
            case TYPE_PAIR(bdld::Datum::e_INTEGER, bdld::Datum::e_DECIMAL64):
                return bdldfp::Decimal64(left.theInteger()) !=
                       right.theDecimal64();
            case TYPE_PAIR(bdld::Datum::e_DECIMAL64, bdld::Datum::e_INTEGER):
                return bdldfp::Decimal64(right.theInteger()) !=
                       left.theDecimal64();
            case TYPE_PAIR(bdld::Datum::e_DOUBLE, bdld::Datum::e_DECIMAL64):
                return notEqual(left.theDouble(), right.theDecimal64());
            case TYPE_PAIR(bdld::Datum::e_DECIMAL64, bdld::Datum::e_DOUBLE):
                return notEqual(right.theDouble(), left.theDecimal64());
            default: {
                bsl::ostringstream error;
                error << "Numbers have incompatible types: " << left << " and "
                      << right;
                throw bdld::Datum::createError(-1, error.str(), d_allocator_p);
            }
        }

#undef TYPE_PAIR
    }
};

}  // namespace

#define DISPATCH(FUNCTION)                                                \
    switch (bdld::Datum::DataType type =                                  \
                normalizeTypes(*args.argsAndOutput, args.allocator)) {    \
        case bdld::Datum::e_INTEGER:                                      \
            return FUNCTION<int>(*args.argsAndOutput, args.allocator);    \
        case bdld::Datum::e_INTEGER64:                                    \
            return FUNCTION<bsls::Types::Int64>(*args.argsAndOutput,      \
                                                args.allocator);          \
        case bdld::Datum::e_DOUBLE:                                       \
            return FUNCTION<double>(*args.argsAndOutput, args.allocator); \
        default:                                                          \
            (void)type;                                                   \
            BSLS_ASSERT(type == bdld::Datum::e_DECIMAL64);                \
            return FUNCTION<bdldfp::Decimal64>(*args.argsAndOutput,       \
                                               args.allocator);           \
    }

void ArithmeticUtil::add(const NativeProcedureUtil::Arguments& args) {
    DISPATCH(homogeneousAdd)
}

void ArithmeticUtil::subtract(const NativeProcedureUtil::Arguments& args) {
    if (args.argsAndOutput->empty()) {
        throw bdld::Datum::createError(
            -1, "substraction requires at least one operand", args.allocator);
    }

    DISPATCH(homogeneousSubtract)
}

void ArithmeticUtil::multiply(const NativeProcedureUtil::Arguments& args) {
    DISPATCH(homogeneousMultiply)
}

void ArithmeticUtil::divide(const NativeProcedureUtil::Arguments& args) {
    if (args.argsAndOutput->empty()) {
        throw bdld::Datum::createError(
            -1, "division requires at least one operand", args.allocator);
    }

    DISPATCH(homogeneousDivide)
}

#undef DISPATCH

void ArithmeticUtil::equal(const NativeProcedureUtil::Arguments& args) {
    if (args.argsAndOutput->empty()) {
        throw bdld::Datum::createError(
            -1,
            "equality comparison requires at least one operand",
            args.allocator);
    }
    if (classify(*args.argsAndOutput).kind ==
        Classification::e_ERROR_NON_NUMERIC_TYPE) {
        throw bdld::Datum::createError(
            -1,
            "equality comparison requires all numeric operands",
            args.allocator);
    }

    bool result;
    if (args.argsAndOutput->size() == 1) {
        result = true;
    }
    else {
        result = std::adjacent_find(args.argsAndOutput->begin(),
                                    args.argsAndOutput->end(),
                                    NotEqual(args.allocator)) ==
                 args.argsAndOutput->end();
    }

    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = bdld::Datum::createBoolean(result);
}

}  // namespace lspcore
