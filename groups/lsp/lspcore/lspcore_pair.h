#ifndef INCLUDED_LSPCORE_PAIR
#define INCLUDED_LSPCORE_PAIR

#include <bdld_datum.h>
#include <bdld_datumudt.h>
#include <bsls_assert.h>
#include <lspcore_userdefinedtypes.h>

namespace BloombergLP {
namespace bslma {
class Allocator;
}  // namespace bslma
}  // namespace BloombergLP

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

struct Pair {
    bdld::Datum first;
    bdld::Datum second;

    Pair(const bdld::Datum& first, const bdld::Datum& second);

    // TODO: document
    static bdld::Datum create(const bdld::Datum& first,
                              const bdld::Datum& second,
                              int                typeOffset,
                              bslma::Allocator*  allocator);

    // TODO: document
    static bool isPair(const bdld::Datum& datum, int typeOffset);

    // TODO: document
    static const Pair& access(const bdld::Datum& pair);
    static const Pair& access(const bdld::DatumUdt& pair);
};

inline Pair::Pair(const bdld::Datum& first, const bdld::Datum& second)
: first(first)
, second(second) {
}

inline bdld::Datum Pair::create(const bdld::Datum& first,
                                const bdld::Datum& second,
                                int                typeOffset,
                                bslma::Allocator*  allocator) {
    return bdld::Datum::createUdt(new (*allocator) Pair(first, second),
                                  UserDefinedTypes::e_PAIR + typeOffset);
}

inline bool Pair::isPair(const bdld::Datum& datum, int typeOffset) {
    return datum.isUdt() &&
           datum.theUdt().type() == UserDefinedTypes::e_PAIR + typeOffset;
}

inline const Pair& Pair::access(const bdld::Datum& pair) {
    // "udt" means "user-defined type"
    BSLS_ASSERT_SAFE(pair.isUdt());

    return access(pair.theUdt());
}

inline const Pair& Pair::access(const bdld::DatumUdt& udt) {
    // "udt" means "user-defined type"
    BSLS_ASSERT_SAFE(udt.data());

    return *static_cast<const Pair*>(udt.data());
}

}  // namespace lspcore

#endif
