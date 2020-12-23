#ifndef INCLUDED_LSPCORE_USERDEFINEDTYPES
#define INCLUDED_LSPCORE_USERDEFINEDTYPES

#include <bsl_cstddef.h>

namespace lspcore {

struct UserDefinedTypes {
    enum Type {
        e_PAIR,
        e_SYMBOL,
        e_PROCEDURE,
        e_NATIVE_PROCEDURE,
        e_SET,
        e_BUILTIN
        // don't forget to update 'k_COUNT'
    };

    static const bsl::size_t k_COUNT = bsl::size_t(e_BUILTIN) + 1;
};

}  // namespace lspcore

#endif
