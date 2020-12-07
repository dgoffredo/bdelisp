#include <lspcore_procedure.h>

namespace lspcore {

Procedure::Procedure(bslma::Allocator* allocator)
: positionalParameters(allocator)
, restParameter(allocator)
, body(0) /* might as well */ {
}

}  // namespace lspcore
