#ifndef INCLUDED_LSPCORE_SYMBOLUTIL
#define INCLUDED_LSPCORE_SYMBOLUTIL

#include <bdld_datum.h>
#include <bdld_datumudt.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_types.h>
#include <lspcore_endian.h>
#include <lspcore_userdefinedtypes.h>

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;
namespace bsls  = BloombergLP::bsls;

struct SymbolUtil {
    // TODO: document
    static bdld::Datum create(const bdld::Datum& stringValue,
                              int                typeOffset,
                              bslma::Allocator*  allocator);
    static bdld::Datum create(bsl::string_view  string,
                              int               typeOffset,
                              bslma::Allocator* allocator);

    // TODO: document
    static bdld::Datum access(const bdld::Datum& symbol);
    static bdld::Datum access(const bdld::DatumUdt& symbol);

    static bool isSymbol(const bdld::Datum& datum, int typeOffset);

  private:
    static bdld::Datum createInPlace(bsl::string_view value, int typeOffset);
    static bdld::Datum createOutOfPlace(const bdld::Datum& stringValue,
                                        int                typeOffset,
                                        bslma::Allocator*  allocator);

    static bdld::Datum accessInPlace(bdld::DatumUdt symbol);
    static bdld::Datum accessOutOfPlace(bdld::DatumUdt symbol);

    static bslma::Allocator* const s_neverAllocate;
};

inline bool SymbolUtil::isSymbol(const bdld::Datum& datum, int typeOffset) {
    return datum.isUdt() &&
           datum.theUdt().type() == UserDefinedTypes::e_SYMBOL + typeOffset;
}

// Symbols containing very small strings are optimized to reside within the
// 'bdld::DatumUdt' itself. One byte of the internal 'void*' is reserved, but
// the other bytes may contain a UTF-8 sequence. On 64-bit systems we can store
// seven bytes of UTF-8. On 32-bit systems we can store three bytes of UTF-8.
//
// Three bytes doesn't seem like much, but it covers:
//
// - single-ASCII identifiers, e.g. "i", "+", "=", ">"
// - "if"
// - "λ" (two bytes in UTF-8)
// - "let"
// - "not"
// - "~>"
//
// It's more useful on 64-bit systems, where up to seven bytes are available:
//
// - "cond"
// - "define"
// - "lambda"
// - "filter"
// - "string?"

// The 'void*' within a 'bdld::DatumUdt' referring to a 'bdld::Datum' string
// will be at least word-aligned, which means the two least significant bits
// will always be zero (on a 32-bit system). We use the least significant bit
// to indicate whether the 'void*' is in fact not a pointer but instead a
// buffer of size 'sizeof(void*) - 1'. Other bits of the least significant byte
// encode the length of the in-place string, and the rest of the bits of the
// least significant byte are reserved for future use.
//
// Where in memory that bit resides, and where the in-place buffer begins,
// depend on whether the platform is little-endian (e.g. x86) or big-endian
// (e.g. SPARC).
//
// See 'lspcore_endian.h'.
//
// If the least significant bit of the least significant byte is set, then this
// is the layout of the bits in that byte (least signifcant bits to the right):
//
// 7   6   5   4     3   2   1     0
// _   _   _   _     _   _   _     1
// ^   ^   ^   ^     ^   ^   ^     ^
//   reserved         length      flag
//
inline bdld::Datum SymbolUtil::create(const bdld::Datum& stringValue,
                                      int                typeOffset,
                                      bslma::Allocator*  allocator) {
    BSLS_ASSERT_SAFE(stringValue.isString());

    const bsl::string_view string = stringValue.theString();

    if (string.size() <= sizeof(void*) - 1) {
        // in-place optimization
        return createInPlace(string, typeOffset);
    }
    return createOutOfPlace(stringValue, typeOffset, allocator);
}

inline bdld::Datum SymbolUtil::create(bsl::string_view  string,
                                      int               typeOffset,
                                      bslma::Allocator* allocator) {
    if (string.size() <= sizeof(void*) - 1) {
        // in-place optimization
        return createInPlace(string, typeOffset);
    }
    return createOutOfPlace(
        bdld::Datum::copyString(string, allocator), typeOffset, allocator);
}

inline bdld::Datum SymbolUtil::createInPlace(bsl::string_view value,
                                             int              typeOffset) {
    BSLS_ASSERT_SAFE(value.size() <= sizeof(void*) - 1);

    char buffer[sizeof(void*)] = {};
    bsl::copy(value.begin(), value.end(), LSPCORE_BEGIN(buffer));
    LSPCORE_LOWBYTE(buffer) = (value.size() << 1) | 1;

    void* fakePtr = *reinterpret_cast<void**>(&buffer[0]);
    return bdld::Datum::createUdt(fakePtr,
                                  UserDefinedTypes::e_SYMBOL + typeOffset);
}

inline bdld::Datum SymbolUtil::createOutOfPlace(const bdld::Datum& stringValue,
                                                int                typeOffset,
                                                bslma::Allocator*  allocator) {
    BSLS_ASSERT_SAFE(stringValue.isString());
    BSLS_ASSERT_SAFE(stringValue.theString().size() >= sizeof(void*));

    return bdld::Datum::createUdt(new (*allocator) bdld::Datum(stringValue),
                                  UserDefinedTypes::e_SYMBOL + typeOffset);
}

inline bdld::Datum SymbolUtil::access(const bdld::Datum& symbol) {
    // udt stands for "user-defined type"
    return access(symbol.theUdt());
}

inline bdld::Datum SymbolUtil::access(const bdld::DatumUdt& udt) {
    // udt stands for "user-defined type"
    if (reinterpret_cast<bsls::Types::UintPtr>(udt.data()) & 1) {
        return accessInPlace(udt);
    }
    return accessOutOfPlace(udt);
}

inline bdld::Datum SymbolUtil::accessInPlace(bdld::DatumUdt symbol) {
    const void* const data = symbol.data();
    // 'bytes' points at the pointer 'data' itself, not what 'data' points to.
    const char* const bytes = reinterpret_cast<const char*>(&data);

    BSLS_ASSERT_SAFE(LSPCORE_LOWBYTE(bytes) & 1);

    const bsl::size_t length = LSPCORE_LOWBYTE(bytes) >> 1;

    // Since 'length' is always less than or equal to
    // 'bdld::Datum::k_SHORTSTRING_SIZE' (a private constant that we can't
    // access), 'copyString' will not use the supplied allocator.
    //
    // However, the contract of 'copyString' implicitly requires that the
    // allocator pointer is not zero, so we have to pass _some_ allocator. We
    // could pass a nonsense pointer, but we can do better.
    //
    // We define a component-private allocator type whose 'allocate' method
    // only asserts that it shall never be called. There is one instance of
    // this allocator in static memory in the implementation of this component.
    return bdld::Datum::copyString(
        LSPCORE_BEGIN(bytes), length, s_neverAllocate);
}

inline bdld::Datum SymbolUtil::accessOutOfPlace(bdld::DatumUdt symbol) {
    const bdld::Datum* const string = static_cast<bdld::Datum*>(symbol.data());

    BSLS_ASSERT_SAFE(string);

    return *string;
}

}  // namespace lspcore

#endif
