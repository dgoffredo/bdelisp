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

struct PrintUtil {
    // TODO: document
    static void print(bsl::ostream&      stream,
                      const bdld::Datum& datum,
                      int                typeOffset);
};

}  // namespace lspcore

#endif
