#include <bsl_functional.h>
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

bool ListUtil::lessThan(
    const bdld::Datum& leftDatum,
    const bdld::Datum& rightDatum,
    const bsl::function<bool(const bdld::Datum&, const bdld::Datum&)>&
        before) {
    bdld::Datum left  = leftDatum;
    bdld::Datum right = rightDatum;

    for (;;) {
        if (left.isNull() && right.isNull()) {
            return false;
        }

        if (left.isNull()) {
            return true;
        }

        if (right.isNull()) {
            return false;
        }

        const Pair& leftPair  = Pair::access(left);
        const Pair& rightPair = Pair::access(right);
        if (before(leftPair.first, rightPair.first)) {
            return true;
        }

        if (before(rightPair.first, leftPair.first)) {
            return false;
        }

        left  = leftPair.second;
        right = rightPair.second;
    }
}

}  // namespace lspcore
