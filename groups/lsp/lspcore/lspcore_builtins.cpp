#include <bdld_datumudt.h>
#include <bsl_cstddef.h>
#include <bsls_assert.h>
#include <lspcore_builtins.h>
#include <lspcore_userdefinedtypes.h>

namespace lspcore {

bool Builtins::isBuiltin(const bdld::Datum& datum, int typeOffset) {
    return datum.isUdt() && isBuiltin(datum.theUdt(), typeOffset);
}

bool Builtins::isBuiltin(const bdld::DatumUdt& udt, int typeOffset) {
    return udt.type() - typeOffset == UserDefinedTypes::e_BUILTIN;
}

Builtins::Builtin Builtins::fromUdtData(void* udtData) {
    const bsl::intptr_t asInt = reinterpret_cast<bsl::intptr_t>(udtData);

    BSLS_ASSERT(asInt >= e_UNDEFINED);
    BSLS_ASSERT(asInt <= e_QUOTE);

    return static_cast<Builtin>(asInt);
}

bdld::Datum Builtins::toDatum(Builtin builtin, int typeOffset) {
    void* const asPtr =
        reinterpret_cast<void*>(static_cast<bsl::intptr_t>(builtin));
    return bdld::Datum::createUdt(asPtr,
                                  UserDefinedTypes::e_BUILTIN + typeOffset);
}

bsl::string_view Builtins::name(Builtin builtin) {
    switch (builtin) {
        // 'e_UNDEFINED' is omitted, because it will never appear in a datum
        // outside of the interpreter's evaluation functions.
        case e_LAMBDA:
            return "λ";
        case e_DEFINE:
            return "define";
        case e_SET:
            return "set!";
        default:
            BSLS_ASSERT_OPT(builtin == e_QUOTE);
            return "quote";
    }
}

}  // namespace lspcore
