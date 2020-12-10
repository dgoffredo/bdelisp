#ifndef INCLUDED_LSPCORE_SYMBOLUTIL
#define INCLUDED_LSPCORE_SYMBOLUTIL

#include <bdld_datum.h>
#include <bdld_datumudt.h>
#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_utility.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <bsls_types.h>
#include <lspcore_endian.h>
#include <lspcore_environment.h>
#include <lspcore_userdefinedtypes.h>

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;
namespace bsls  = BloombergLP::bsls;

struct SymbolUtil {
    static bdld::Datum create(const bdld::Datum& stringValue,
                              int                typeOffset,
                              bslma::Allocator*  allocator);
    static bdld::Datum create(bsl::string_view  string,
                              int               typeOffset,
                              bslma::Allocator* allocator);
    static bdld::Datum create(
        const bsl::pair<const bsl::string, bdld::Datum>& entry,
        int                                              typeOffset);
    static bdld::Datum create(bsl::size_t argumentsOffset, int typeOffset);

    // Return a 'Datum' containing or referring to a string that is the name
    // associated with the specified 'symbol'. The behavior is undefined if
    // 'symbol' is encoded as an offset to an environment argument vector. Note
    // that the overloads of 'name' that take an 'Environment' parameter do not
    // have this limitation.
    // TODO: What about lifetime? (referring to a 'bsl::string' in some env)
    static bdld::Datum name(const bdld::Datum& symbol);
    static bdld::Datum name(const bdld::DatumUdt& symbol);

    // Return a 'Datum' containing or referring to a string that is the name
    // associated with the specified 'symbol' in the specified 'environment'.
    // TODO: What about lifetime? (referring to a 'bsl::string' in some env)
    static bdld::Datum name(const bdld::Datum& symbol,
                            const Environment& environment);
    static bdld::Datum name(const bdld::DatumUdt& symbol,
                            const Environment&    environment);

    // Return a pointer to the environment entry to which 'symbol' refers in
    // the specified 'environment'. Return a null pointer if 'symbol' is
    // unbound in 'environment'. The behavior is undefined if 'symbol' is
    // encoded as an environment entry pointer or as an argument vector offset
    // for an environment other than 'environment'.
    static const bsl::pair<const bsl::string, bdld::Datum>* resolve(
        const bdld::Datum& symbol, const Environment& environment);
    static const bsl::pair<const bsl::string, bdld::Datum>* resolve(
        const bdld::DatumUdt& symbol, const Environment& environment);

    static bool isSymbol(const bdld::Datum& datum, int typeOffset);

  private:
    enum Encoding { e_DATUM_PTR, e_IN_PLACE, e_ENTRY_PTR, e_ARGUMENTS_OFFSET };

    static Encoding encoding(void* udtData);

    static bdld::Datum createInPlace(bsl::string_view value, int typeOffset);
    static bdld::Datum createOutOfPlace(const bdld::Datum& stringValue,
                                        int                typeOffset,
                                        bslma::Allocator*  allocator);

    static bdld::Datum nameInPlace(void* udtData);
    static bdld::Datum nameOutOfPlace(void* udtData);
    static bdld::Datum nameEntry(void* udtData);
    static bdld::Datum nameOffset(void* udtData, const Environment&);

    static const bsl::pair<const bsl::string, bdld::Datum>* entry(
        void* udtData);
    static const bsl::pair<const bsl::string, bdld::Datum>* fromOffset(
        void* udtData, const Environment&);

    static bslma::Allocator* const s_neverAllocate;
};

inline bool SymbolUtil::isSymbol(const bdld::Datum& datum, int typeOffset) {
    return datum.isUdt() &&
           datum.theUdt().type() == UserDefinedTypes::e_SYMBOL + typeOffset;
}

// Symbols have four different representations:
//
// 1. 'bdld::Datum*' where the datum has type 'bdld::Datum::e_STRING'
// 2. tiny in-place string
// 3. 'bsl::pair<bsl::string, bdld::Datum>*' to a resolved environment entry
// 4. an offset into the local environment's vector of procedure arguments
//
// In all cases, the length of the name of a symbol must not exceed
// 65,535 bytes.
//
// For (2), symbols containing very small strings are optimized to reside
// within the 'bdld::DatumUdt' itself. One byte of the internal 'void*' is
// reserved, but the other bytes may contain a UTF-8 sequence. On 64-bit
// systems we can store seven bytes of UTF-8. On 32-bit systems we can store
// three bytes of UTF-8.
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

// The 'void*' within a 'bdld::DatumUdt' referring to a 'bdld::Datum' string or
// a 'bsl::pair<bsl::string, bdld::Datum>' will be at least word-aligned, which
// means the two least significant bits will always be zero (on a 32-bit
// system). Thus a symbol can have up to four distinct representations.
//
// Where in memory the two "selector" bits reside, and where the in-place
// buffer (if any) begins, depend on whether the platform is little-endian
// (e.g. x86) or big-endian (e.g. SPARC).
//
// See 'lspcore_endian.h'.
//
// The interpretation of the bits within the lest significant byte depends upon
// the values of the least significant byte's two least significant bits:
//
//
//     bit position:    7   6   5   4   3   2       1   0
//     bit value:       _   _   _   _   _   _       0   0
//                      ^   ^   ^   ^   ^   ^       ^   ^
//                      bdld::Datum-string-ptr     selector
//
//
//     bit position:    7   6   5     4   3   2     1   0
//     bit value:       _   _   _     _   _   _     0   1
//                      ^   ^   ^     ^   ^   ^     ^   ^
//                      reserved       length      selector
//
//
//     bit position:    7   6   5   4   3   2       1   0
//     bit value:       _   _   _   _   _   _       1   0
//                      ^   ^   ^   ^   ^   ^       ^   ^
//                      environment-entry-ptr      selector
//
//
//     bit position:    7   6   5   4   3   2       1   0
//     bit value:       _   _   _   _   _   _       1   1
//                      ^   ^   ^   ^   ^   ^       ^   ^
//                      argument-offset-index      selector
//
//
// In cases (1), (3), and (4) -- so, all but in-place string -- the
// pointer/index continues for the rest of the bits of the word.

inline bdld::Datum SymbolUtil::create(const bdld::Datum& stringValue,
                                      int                typeOffset,
                                      bslma::Allocator*  allocator) {
    BSLS_ASSERT(stringValue.isString());

    const bsl::string_view string = stringValue.theString();

    BSLS_ASSERT(string.size() < 65536);

    if (string.size() <= sizeof(void*) - 1) {
        // in-place optimization
        return createInPlace(string, typeOffset);
    }
    return createOutOfPlace(stringValue, typeOffset, allocator);
}

inline bdld::Datum SymbolUtil::create(bsl::string_view  string,
                                      int               typeOffset,
                                      bslma::Allocator* allocator) {
    BSLS_ASSERT(string.size() < 65536);

    if (string.size() <= sizeof(void*) - 1) {
        // in-place optimization
        return createInPlace(string, typeOffset);
    }
    return createOutOfPlace(
        bdld::Datum::copyString(string, allocator), typeOffset, allocator);
}

inline bdld::Datum SymbolUtil::create(
    const bsl::pair<const bsl::string, bdld::Datum>& entry, int typeOffset) {
    void* const fakePtr = reinterpret_cast<void*>(
        reinterpret_cast<bsls::Types::UintPtr>(&entry) | e_ENTRY_PTR);

    return bdld::Datum::createUdt(fakePtr,
                                  UserDefinedTypes::e_SYMBOL + typeOffset);
}

inline bdld::Datum SymbolUtil::create(bsl::size_t argumentsOffset,
                                      int         typeOffset) {
    void* const fakePtr =
        reinterpret_cast<void*>((argumentsOffset << 2) | e_ARGUMENTS_OFFSET);

    return bdld::Datum::createUdt(fakePtr,
                                  UserDefinedTypes::e_SYMBOL + typeOffset);
}

inline bdld::Datum SymbolUtil::createInPlace(bsl::string_view value,
                                             int              typeOffset) {
    BSLS_ASSERT(value.size() <= sizeof(void*) - 1);

    char buffer[sizeof(void*)] = {};
    bsl::copy(value.begin(), value.end(), LSPCORE_BEGIN(buffer));
    LSPCORE_LOWBYTE(buffer) = (value.size() << 2) | e_IN_PLACE;

    void* fakePtr = *reinterpret_cast<void**>(&buffer[0]);
    return bdld::Datum::createUdt(fakePtr,
                                  UserDefinedTypes::e_SYMBOL + typeOffset);
}

inline bdld::Datum SymbolUtil::createOutOfPlace(const bdld::Datum& stringValue,
                                                int                typeOffset,
                                                bslma::Allocator*  allocator) {
    BSLS_ASSERT(e_DATUM_PTR == 0);
    BSLS_ASSERT(stringValue.isString());
    BSLS_ASSERT(stringValue.theString().size() >= sizeof(void*));

    return bdld::Datum::createUdt(new (*allocator) bdld::Datum(stringValue),
                                  UserDefinedTypes::e_SYMBOL + typeOffset);
}

inline bdld::Datum SymbolUtil::name(const bdld::Datum& symbol) {
    BSLS_ASSERT(symbol.isUdt());

    // udt stands for "user-defined type"
    return name(symbol.theUdt());
}

inline bdld::Datum SymbolUtil::name(const bdld::Datum& symbol,
                                    const Environment& environment) {
    BSLS_ASSERT(symbol.isUdt());

    // udt stands for "user-defined type"
    return name(symbol.theUdt(), environment);
}

inline bdld::Datum SymbolUtil::name(const bdld::DatumUdt& udt) {
    // udt stands for "user-defined type"
    switch (const Encoding format = encoding(udt.data())) {
        case e_DATUM_PTR:
            return nameOutOfPlace(udt.data());
        case e_IN_PLACE:
            return nameInPlace(udt.data());
        default:
            (void)format;
            BSLS_ASSERT(format == e_ENTRY_PTR);
            // 'e_ARGUMENTS_OFFSET' is disallowed, because we don't have an
            // 'Environment' where we can look up the name.
            return nameEntry(udt.data());
    }
}

inline bdld::Datum SymbolUtil::name(const bdld::DatumUdt& udt,
                                    const Environment&    environment) {
    // udt stands for "user-defined type"
    switch (const Encoding format = encoding(udt.data())) {
        case e_DATUM_PTR:
            return nameOutOfPlace(udt.data());
        case e_IN_PLACE:
            return nameInPlace(udt.data());
        case e_ENTRY_PTR:
            return nameEntry(udt.data());
        default:
            (void)format;
            BSLS_ASSERT(format == e_ARGUMENTS_OFFSET);
            return nameOffset(udt.data(), environment);
    }
}

inline SymbolUtil::Encoding SymbolUtil::encoding(void* udtData) {
    // 0000...0000011, i.e. just the lowest two bits
    const bsls::Types::UintPtr mask = bsls::Types::UintPtr(3);

    return SymbolUtil::Encoding(
        reinterpret_cast<bsls::Types::UintPtr>(udtData) & mask);
}

inline bdld::Datum SymbolUtil::nameInPlace(void* udtData) {
    BSLS_ASSERT(encoding(udtData) == e_IN_PLACE);

    // 'bytes' points at the pointer 'data' itself, not what 'data' points to.
    const char* const bytes  = reinterpret_cast<const char*>(&udtData);
    const bsl::size_t length = LSPCORE_LOWBYTE(bytes) >> 2;

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

inline bdld::Datum SymbolUtil::nameOutOfPlace(void* udtData) {
    const bdld::Datum* const string = static_cast<bdld::Datum*>(udtData);

    BSLS_ASSERT(string);

    return *string;
}

inline const bsl::pair<const bsl::string, bdld::Datum>* SymbolUtil::entry(
    void* udtData) {
    BSLS_ASSERT(encoding(udtData) == e_ENTRY_PTR);

    // 11111...11111100, i.e. unset the lowest two bits.
    const bsls::Types::UintPtr mask = ~bsls::Types::UintPtr(3);

    const void* const realPtr = reinterpret_cast<const void*>(
        reinterpret_cast<bsls::Types::UintPtr>(udtData) & mask);

    const bsl::pair<const bsl::string, bdld::Datum>* const result =
        static_cast<const bsl::pair<const bsl::string, bdld::Datum>*>(realPtr);

    BSLS_ASSERT(result);
    return result;
}

inline const bsl::pair<const bsl::string, bdld::Datum>* SymbolUtil::fromOffset(
    void* udtData, const Environment& environment) {
    BSLS_ASSERT(encoding(udtData) == e_ARGUMENTS_OFFSET);

    const bsls::Types::UintPtr offset =
        reinterpret_cast<bsls::Types::UintPtr>(udtData) >> 2;

    BSLS_ASSERT(environment.arguments().size() < offset);
    BSLS_ASSERT(environment.arguments()[offset]);

    return environment.arguments()[offset];
}

inline bdld::Datum SymbolUtil::nameEntry(void* udtData) {
    // TODO: What about lifetime? (referring to a 'bsl::string' in some env)
    // Right now I'm not copying, because I figure it will be fine; but when
    // you look into garbage collection more closely, look at this again.
    return bdld::Datum::createStringRef(entry(udtData)->first,
                                        s_neverAllocate);
}

inline bdld::Datum SymbolUtil::nameOffset(void*              udtData,
                                          const Environment& environment) {
    // TODO: What about lifetime? (referring to a 'bsl::string' in some env)
    // Right now I'm not copying, because I figure it will be fine; but when
    // you look into garbage collection more closely, look at this again.
    return bdld::Datum::createStringRef(
        fromOffset(udtData, environment)->first, s_neverAllocate);
}

inline const bsl::pair<const bsl::string, bdld::Datum>* SymbolUtil::resolve(
    const bdld::Datum& symbol, const Environment& environment) {
    BSLS_ASSERT(symbol.isUdt());

    return resolve(symbol.theUdt(), environment);
}

inline const bsl::pair<const bsl::string, bdld::Datum>* SymbolUtil::resolve(
    const bdld::DatumUdt& symbol, const Environment& environment) {
    switch (const Encoding format = encoding(symbol.data())) {
        case e_DATUM_PTR:  // fall through
        case e_IN_PLACE:
            return environment.lookup(name(symbol).theString());
        case e_ENTRY_PTR:
            return entry(symbol.data());
        default:
            (void)format;
            BSLS_ASSERT(format == e_ARGUMENTS_OFFSET);
            return fromOffset(symbol.data(), environment);
    }
}

}  // namespace lspcore

#endif
