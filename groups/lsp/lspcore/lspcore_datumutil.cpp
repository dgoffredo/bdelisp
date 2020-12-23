#include <bdld_datum.h>
#include <bsl_algorithm.h>
#include <bsl_cstring.h>
#include <lspcore_datumutil.h>
#include <lspcore_listutil.h>
#include <lspcore_pair.h>
#include <lspcore_userdefinedtypes.h>

using namespace BloombergLP;

namespace lspcore {
namespace {

bool isNumeric(const bdld::Datum& value) {
    switch (value.type()) {
        case bdld::Datum::e_INTEGER:
        case bdld::Datum::e_DOUBLE:
        case bdld::Datum::e_INTEGER64:
        case bdld::Datum::e_DECIMAL64:
            return true;
        default:
            return false;
    }
}

bool intLessThanNumeric(int left, const bdld::Datum& right) {
    switch (right.type()) {
        case bdld::Datum::e_INTEGER:
            return left < right.theInteger();
        case bdld::Datum::e_DOUBLE:
            return left < right.theDouble();
        case bdld::Datum::e_INTEGER64:
            return left < right.theInteger64();
        default:
            BSLS_ASSERT(right.type() == bdld::Datum::e_DECIMAL64);
            return bdldfp::Decimal64(left) < right.theDecimal64();
    }
}

bool numericLessThanInt(const bdld::Datum& left, int right) {
    switch (left.type()) {
        case bdld::Datum::e_INTEGER:
            return left.theInteger() < right;
        case bdld::Datum::e_DOUBLE:
            return left.theDouble() < right;
        case bdld::Datum::e_INTEGER64:
            return left.theInteger64() < right;
        default:
            BSLS_ASSERT(right.type() == bdld::Datum::e_DECIMAL64);
            return left.theDecimal64() < bdldfp::Decimal64(right);
    }
}

class LessThan {
    int typeOffset;

  public:
    explicit LessThan(int typeOffset)
    : typeOffset(typeOffset) {
    }

    bool operator()(const bdld::Datum& left, const bdld::Datum& right) const {
        // The general rule is that 'left' and 'right' are compared first by
        // their types, and if they have the same type then by their values.
        // One special case is for comparisons between 'int' and a non-'int'
        // numeric, e.g. between 'int' and 'double'. In that case, the 'int' is
        // promoted to the other numeric type and then compared by value.
        // TODO: Mixing decimal64 and double is a mistake waiting to happen...
        //
        // Note that since 'bdld::Datum::e_NIL' is the smallest
        // 'bdld::Datum::DataType', empty lists and empty maps (null) are less
        // than their non-empty counterparts, as expected.
        if (left.isInteger() && isNumeric(right)) {
            return intLessThanNumeric(left.theInteger(), right);
        }
        if (right.isInteger() && isNumeric(left)) {
            return numericLessThanInt(left, right.theInteger());
        }

        if (left.type() < right.type()) {
            return true;
        }
        if (right.type() < left.type()) {
            return false;
        }

        // 'left' and 'right' have the same type. Compare by value.
        switch (left.type()) {
            case bdld::Datum::e_NIL:
                return false;
            case bdld::Datum::e_INTEGER:
                return left.theInteger() < right.theInteger();
            case bdld::Datum::e_DOUBLE:
                return left.theDouble() < right.theDouble();
            case bdld::Datum::e_STRING:
                return left.theString() < right.theString();
            case bdld::Datum::e_BOOLEAN:
                return int(left.theBoolean()) < int(right.theBoolean());
            case bdld::Datum::e_ERROR: {
                const bdld::DatumError leftError  = left.theError();
                const bdld::DatumError rightError = right.theError();
                if (leftError.code() == rightError.code()) {
                    return leftError.message() < rightError.message();
                }
                return leftError.code() < rightError.code();
            }
            case bdld::Datum::e_DATE:
                return left.theDate() < right.theDate();
            case bdld::Datum::e_TIME:
                return left.theTime() < right.theTime();
            case bdld::Datum::e_DATETIME:
                return left.theDatetime() < right.theDatetime();
            case bdld::Datum::e_DATETIME_INTERVAL:
                return left.theDatetimeInterval() <
                       right.theDatetimeInterval();
            case bdld::Datum::e_INTEGER64:
                return left.theInteger64() < right.theInteger64();
            case bdld::Datum::e_USERDEFINED: {
                const bdld::DatumUdt leftUdt  = left.theUdt();
                const bdld::DatumUdt rightUdt = right.theUdt();
                if (leftUdt.type() != rightUdt.type()) {
                    return leftUdt.type() < rightUdt.type();
                }

                switch (leftUdt.type() - typeOffset) {
                    case UserDefinedTypes::e_PAIR:
                        return ListUtil::lessThan(left, right, *this);
                    case UserDefinedTypes::e_SET:
                        // TODO: walky walky
                    case UserDefinedTypes::e_SYMBOL:
                        // TODO: by name? uh oh... what about bound symbols
                        // TODO: AUGH I SHOULD JUST UNDO THAT SPACE
                        // OPTIMIZATION
                    case UserDefinedTypes::e_PROCEDURE:
                    case UserDefinedTypes::e_NATIVE_PROCEDURE:
                    case UserDefinedTypes::e_BUILTIN:
                    default:
                        // There's no value semantic related ordering, so just
                        // order the representations in memory.
                        return leftUdt.data() < rightUdt.data();
                }
            }
            case bdld::Datum::e_ARRAY: {
                const bdld::DatumArrayRef leftArray  = left.theArray();
                const bdld::DatumArrayRef rightArray = right.theArray();

                return bsl::lexicographical_compare(
                    leftArray.data(),
                    leftArray.data() + leftArray.length(),
                    rightArray.data(),
                    rightArray.data() + rightArray.length(),
                    *this);
            }
            case bdld::Datum::e_MAP: {
                const bdld::DatumMapRef leftMap  = left.theMap();
                const bdld::DatumMapRef rightMap = right.theMap();

                return bsl::lexicographical_compare(
                    leftMap.data(),
                    leftMap.data() + leftMap.size(),
                    rightMap.data(),
                    rightMap.data() + rightMap.size(),
                    *this);
            }
            case bdld::Datum::e_BINARY: {
                const bdld::DatumBinaryRef leftBinary  = left.theBinary();
                const bdld::DatumBinaryRef rightBinary = right.theBinary();
                bsl::size_t                shorter =
                    bsl::min(leftBinary.size(), rightBinary.size());
                const int comparison = bsl::memcmp(
                    leftBinary.data(), rightBinary.data(), shorter);
                if (comparison < 0) {
                    return true;
                }
                if (comparison > 0) {
                    return false;
                }
                return leftBinary.size() < shorter;
            }
            case bdld::Datum::e_DECIMAL64:
                return left.theDecimal64() < right.theDecimal64();
            default: {
                BSLS_ASSERT_OPT(left.type() == bdld::Datum::e_INT_MAP);
                const bdld::DatumIntMapRef leftMap  = left.theIntMap();
                const bdld::DatumIntMapRef rightMap = right.theIntMap();

                return bsl::lexicographical_compare(
                    leftMap.data(),
                    leftMap.data() + leftMap.size(),
                    rightMap.data(),
                    rightMap.data() + rightMap.size(),
                    *this);
            }
        }
    }

    // This works for both 'bdld::DatumMapEntry' and 'bdld::DatumIntMapEntry'.
    template <typename MapEntry>
    bool operator()(const MapEntry& left, const MapEntry& right) const {
        if (left.key() < right.key()) {
            return true;
        }
        if (right.key() < left.key()) {
            return false;
        }
        return (*this)(left.value(), right.value());
    }
};

}  // namespace

DatumUtil::Comparator DatumUtil::lessThanComparator(int typeOffset) {
    return LessThan(typeOffset);
}

}  // namespace lspcore
