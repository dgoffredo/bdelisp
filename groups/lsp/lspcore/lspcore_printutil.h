#ifndef INCLUDED_LSPCORE_PRINTUTIL
#define INCLUDED_LSPCORE_PRINTUTIL

#include <bsl_iosfwd.h>

namespace BloombergLP {
namespace bdld {
class Datum;
}  // namespace bdld
}  // namespace BloombergLP

namespace lspcore {
namespace bdld = BloombergLP::bdld;

class Pair;

struct PrintUtil {
    // TODO: document
    static void print(bsl::ostream&      stream,
                      const bdld::Datum& datum,
                      int                typeOffset);
    static void print(bsl::ostream& stream, const Pair& pair, int typeOffset);
};

}  // namespace lspcore

#endif
