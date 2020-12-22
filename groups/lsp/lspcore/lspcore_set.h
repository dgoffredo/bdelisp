#ifndef INCLUDED_LSPCORE_SET
#define INCLUDED_LSPCORE_SET

// A set is an immutable AVL Tree (https://en.wikipedia.org/wiki/AVL_tree) of
// distinct values. A set is represented as a pointer to a 'Set' instance. A
// null pointer denotes an empty set, while a 'Set' instance denotes a set with
// a least one element. A 'Set' object contains a 'value' member, which is an
// element in the set, and 'left' and 'right' subtrees.
//
// 'Set' objects cache the maximum height of the subtrees below them (plus
// one). On 64-bit systems, this height is stored in the lower bits of the
// 'left' and 'right' pointer members of the 'Set'. On 32-bit systems, the
// height is stored in its own integer.
//
// 'Set' partitions its elements into equivalency classes based on a specified
// comparator predicate 'before'. For elements 'a' and 'b':
//
// - 'a' is before 'b' if 'before(a, b)'.
// - 'a' is after 'b' if 'before(b, a)'.
// - 'a' and 'b' are equivalent if neither 'before(a, b)' nor 'before(b, a)'.
//
// The comparator ('before') is an additional argument to functions that need
// it.

#include <bdld_datum.h>
#include <bsl_functional.h>
#include <bsls_platform.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bslma {
class Allocator;
}  // namespace bslma
}  // namespace BloombergLP

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;
namespace bsls  = BloombergLP::bsls;

class Set {
    bdld::Datum d_value;
    const Set*  d_left;
    const Set*  d_right;

#ifdef BSLS_PLATFORM_CPU_32_BIT
    int d_height;
#else
    static bsls::Types::UintPtr asInt(const Set*);
    static const Set*           asPtr(bsls::Types::UintPtr);
#endif

    void setHeight(int);

  public:
    Set(const bdld::Datum& value, const Set* left, const Set* right);

    int        height() const;
    static int height(const Set*);

    const bdld::Datum& value() const;
    const Set*         left() const;
    const Set*         right() const;

    typedef bsl::function<bool(const bdld::Datum&, const bdld::Datum&)>
        Comparator;

    static const Set* insert(const Set*         set,
                             const bdld::Datum& value,
                             const Comparator&  before,
                             bslma::Allocator*);

    static bool contains(const Set*         set,
                         const bdld::Datum& value,
                         const Comparator&  before);

    static const Set* remove(const Set*         set,
                             const bdld::Datum& value,
                             const Comparator&  before,
                             bslma::Allocator*  allocator);

    static bdld::Datum toList(const Set* set,
                              int        typeOffset,
                              bslma::Allocator*);
};

#ifdef BSLS_PLATFORM_CPU_64_BIT
inline bsls::Types::UintPtr Set::asInt(const Set* ptr) {
    return reinterpret_cast<bsls::Types::UintPtr>(ptr);
}

inline const Set* Set::asPtr(bsls::Types::UintPtr bits) {
    return reinterpret_cast<const Set*>(bits);
}
#endif

inline int Set::height() const {
#ifdef BSLS_PLATFORM_CPU_32_BIT
    return d_height;
#else
    // 6-bit integer: the lowest three bits of 'left' and the lowest three bits
    // of 'right'
    return ((asInt(d_left) & 7) << 3) | (asInt(d_right) & 7);
#endif
}

inline int Set::height(const Set* set) {
    if (set) {
        return set->height();
    }

    return 0;
}

inline const bdld::Datum& Set::value() const {
    return d_value;
}

inline const Set* Set::left() const {
#ifdef BSLS_PLATFORM_CPU_32_BIT
    return d_left;
#else
    const bsls::Types::UintPtr mask = ~bsls::Types::UintPtr(7);
    return asPtr(asInt(d_left) & mask);
#endif
}

inline const Set* Set::right() const {
#ifdef BSLS_PLATFORM_CPU_32_BIT
    return d_right;
#else
    const bsls::Types::UintPtr mask = ~bsls::Types::UintPtr(7);
    return asPtr(asInt(d_right) & mask);
#endif
}

}  // namespace lspcore

#endif
