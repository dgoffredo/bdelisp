#ifndef INCLUDED_LSPCORE_DATUMUTIL
#define INCLUDED_LSPCORE_DATUMUTIL

#include <bsl_functional.h>

namespace BloombergLP {
namespace bdld {
class Datum;
}  // namespace bdld
}  // namespace BloombergLP

namespace lspcore {
namespace bdld = BloombergLP::bdld;

struct DatumUtil {
    typedef bsl::function<bool(const bdld::Datum&, const bdld::Datum&)>
        Comparator;

    static Comparator lessThanComparator(int typeOffset);
};

}  // namespace lspcore

#endif
