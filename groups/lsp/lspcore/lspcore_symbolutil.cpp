#include <lspcore_symbolutil.h>

using namespace BloombergLP;

namespace lspcore {
namespace {

class NeverAllocate : public bslma::Allocator {
  public:
    ~NeverAllocate();

    void* allocate(bsl::size_t);

    void deallocate(void*);
};

NeverAllocate::~NeverAllocate() {
}

void* NeverAllocate::allocate(bsl::size_t) {
    BSLS_ASSERT_OPT(!"NeverAllocate never allocates");
    return 0;
}

void NeverAllocate::deallocate(void*) {
    BSLS_ASSERT_OPT(!"NeverAllocate never (de)allocates");
}

NeverAllocate s_neverAllocateInstance;

}  // namespace

bslma::Allocator* const SymbolUtil::s_neverAllocate = &s_neverAllocateInstance;

}  // namespace lspcore
