#include <bsl_iterator.h>
#include <bsls_assert.h>
#include <lspcore_listutil.h>
#include <lspcore_pair.h>

namespace lspcore {

bdld::Datum ListUtil::createList(const bdld::Datum* begin,
                                 const bdld::Datum* end,
                                 int                typeOffset,
                                 bslma::Allocator*  allocator) {
    typedef bsl::reverse_iterator<const bdld::Datum*> Iter;
    const Iter                                        rbegin = Iter(end);
    const Iter                                        rend   = Iter(begin);

    bdld::Datum list = bdld::Datum::createNull();

    for (Iter iter = rbegin; iter != rend; ++iter) {
        list = Pair::create(*iter, list, typeOffset, allocator);
    }

    return list;
}

bdld::Datum ListUtil::createImproperList(const bdld::Datum* begin,
                                         const bdld::Datum* end,
                                         int                typeOffset,
                                         bslma::Allocator*  allocator) {
    BSLS_ASSERT(end - begin >= 2);

    typedef bsl::reverse_iterator<const bdld::Datum*> Iter;
    const Iter                                        rbegin = Iter(end);
    const Iter                                        rend   = Iter(begin);

    typedef bsl::vector<bdld::Datum>::const_reverse_iterator Iter;
    Iter                                                     iter = rbegin;

    // the last two elements are definitely paired together
    bdld::Datum improperList = *iter;
    ++iter;
    improperList = Pair::create(*iter, improperList, typeOffset, allocator);

    // and there might be more
    while (++iter != rend) {
        improperList =
            Pair::create(*iter, improperList, typeOffset, allocator);
    }

    return improperList;
}

bool ListUtil::isProperList(const bdld::Datum& datum, int typeOffset) {
    bdld::Datum rest = datum;
    while (Pair::isPair(rest, typeOffset)) {
        rest = Pair::access(rest).second;
    }
    return rest.isNull();
}

}  // namespace lspcore
