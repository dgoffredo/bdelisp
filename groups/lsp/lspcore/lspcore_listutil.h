#ifndef INCLUDED_LSPCORE_LISTUTIL
#define INCLUDED_LSPCORE_LISTUTIL

#include <bdld_datum.h>

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

    // Return a 'Pair' that is the head of an improper list containing the
    // specified 'elements'. An improper list is one whose final 'Pair::second'
    // is the last element, e.g.
    //
    //     (improper-list a b c d)  →  (pair a (pair b (pair c d)))
    //
    // The behavior is undefined unless 'elements.size() >= 2'.
    // 'bdld::DatumUdt' objects created by this function will have their
    // '.type' incremented by the specified 'typeOffset'. Use the specified
    // 'allocator' to supply memory.
    static bdld::Datum createImproperList(
        const bsl::vector<bdld::Datum>& elements,
        int                             typeOffset,
        bslma::Allocator*               allocator);

    // TODO: no
    static void hackPrint(const bdld::Datum& list);
};

}  // namespace lspcore

#endif
