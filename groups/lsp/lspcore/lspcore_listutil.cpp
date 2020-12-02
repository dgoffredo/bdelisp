#include <bsl_iostream.h>  // TODO: no
#include <bsls_assert.h>
#include <lspcore_listutil.h>
#include <lspcore_pair.h>

namespace lspcore {

bdld::Datum ListUtil::createList(const bsl::vector<bdld::Datum>& elements,
                                 int                             typeOffset,
                                 bslma::Allocator*               allocator) {
    typedef bsl::vector<bdld::Datum>::const_reverse_iterator Iter;

    bdld::Datum list = bdld::Datum::createNull();

    for (Iter iter = elements.rbegin(); iter != elements.rend(); ++iter) {
        list = Pair::create(*iter, list, typeOffset, allocator);
    }

    return list;
}

bdld::Datum ListUtil::createImproperList(
    const bsl::vector<bdld::Datum>& elements,
    int                             typeOffset,
    bslma::Allocator*               allocator) {
    BSLS_ASSERT(elements.size() >= 2);

    typedef bsl::vector<bdld::Datum>::const_reverse_iterator Iter;
    Iter iter = elements.rbegin();

    // the last two elements are definitely paired together
    bdld::Datum improperList = *iter;
    ++iter;
    improperList = Pair::create(*iter, improperList, typeOffset, allocator);

    // and there might be more
    while (++iter != elements.rend()) {
        improperList =
            Pair::create(*iter, improperList, typeOffset, allocator);
    }

    return improperList;
}

void ListUtil::hackPrint(const bdld::Datum& list) {
    bsl::cout << "(";

    bdld::Datum item = list;
    if (!item.isNull()) {
        const Pair& pair = Pair::access(item);
        bsl::cout << pair.first;
        item = pair.second;

        while (!item.isNull()) {
            const int typeOffset = 0;
            if (Pair::isPair(item, typeOffset)) {
                const Pair& pair = Pair::access(item);
                bsl::cout << " " << pair.first;
                item = pair.second;
            }
            else {
                bsl::cout << " . " << item;
                break;
            }
        }
    }

    bsl::cout << ")";
}

}  // namespace lspcore
