#ifndef INCLUDED_LSPCORE_LISTUTIL
#define INCLUDED_LSPCORE_LISTUTIL

#include <bdld_datum.h>
#include <bsl_functional.h>

namespace BloombergLP {
namespace bslma {
class Allocator;
}  // namespace bslma
}  // namespace BloombergLP

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

struct ListUtil {
    // Return a 'Pair' that is the head of a list containing the specified
    // 'elements'. A proper list is one whose final 'Pair::first' is the last
    // element, and whose final 'Pair::second' is null, e.g.
    //
    //     (proper-list a b c d)  →  (pair a (pair b (pair c (pair d ()))))
    //
    // Return null if 'elements' is empty. 'bdld::DatumUdt' objects created by
    // this function will have their '.type' incremented by the specified
    // 'typeOffset'. Use the specified 'allocator' to supply memory.
    static bdld::Datum createList(const bsl::vector<bdld::Datum>& elements,
                                  int                             typeOffset,
                                  bslma::Allocator*               allocator);
    static bdld::Datum createList(const bdld::Datum* begin,
                                  const bdld::Datum* end,
                                  int                typeOffset,
                                  bslma::Allocator*  allocator);

    // Return a 'Pair' that is the head of an improper list containing the
    // specified 'elements'. An improper list is one whose final 'Pair::second'
    // is the last element, e.g.
    //
    //     (improper-list a b c d)  →  (pair a (pair b (pair c d)))
    //
    // The behavior is undefined unless 'elements.size() >= 2'.
    // 'bdld::DatumUdt' objects created by this function will have their
    // '.type' incremented by the specified 'typeOffset'. Use the specified
    // 'allocator' to supply memory. Note that even though this function is
    // called 'createImproperList', the resulting 'Pair' might still be a
    // proper list, iff the final element in 'elements' is a list.
    static bdld::Datum createImproperList(
        const bsl::vector<bdld::Datum>& elements,
        int                             typeOffset,
        bslma::Allocator*               allocator);
    static bdld::Datum createImproperList(const bdld::Datum* begin,
                                          const bdld::Datum* end,
                                          int                typeOffset,
                                          bslma::Allocator*  allocator);

    // Return whether the specified 'pair' is a proper list. A proper list is a
    // composition of pairs whose "end" is null, e.g.
    //
    //     (pair 1 (pair 2 (pair 3 (pair 4 ()))))
    //
    // is the proper list
    //
    //     (1 2 3 4)
    //
    // The salient property is that 4 is paired with null (which is also the
    // empty list).
    //
    // Use the specified 'typeOffset' when determining the user-defined type
    // code for a 'Pair'.
    static bool isProperList(const bdld::Datum& datum, int typeOffset);

    // Return whether the specified 'left' is lexicographically less than the
    // specified 'right'. Use the specified 'before' to determine when one
    // element is less than another. The behavior is undefined unless each of
    // 'left' and 'right' is either a 'Pair' or null.
    static bool lessThan(
        const bdld::Datum& left,
        const bdld::Datum& right,
        const bsl::function<bool(const bdld::Datum&, const bdld::Datum&)>&
            before);
};

inline bdld::Datum ListUtil::createList(
    const bsl::vector<bdld::Datum>& elements,
    int                             typeOffset,
    bslma::Allocator*               allocator) {
    return createList(elements.begin(), elements.end(), typeOffset, allocator);
}

inline bdld::Datum ListUtil::createImproperList(
    const bsl::vector<bdld::Datum>& elements,
    int                             typeOffset,
    bslma::Allocator*               allocator) {
    return createImproperList(
        elements.begin(), elements.end(), typeOffset, allocator);
}

}  // namespace lspcore

#endif
