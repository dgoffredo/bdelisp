#ifndef INCLUDED_LSPCORE_BUILTINS
#define INCLUDE_LSPCORE_BUILTINS

#include <bdld_datum.h>
#include <bsl_string_view.h>

namespace lspcore {
namespace bdld = BloombergLP::bdld;

struct Builtins {
    enum Builtin {
        e_UNDEFINED,  // used by the interpreter implementation only
        e_LAMBDA,
        e_DEFINE,
        e_SET,  // as in (set! foo bar)
        e_QUOTE
        // don't forget to update the definition of 'fromUdtData'
    };

    static bool isBuiltin(const bdld::Datum& datum, int typeOffset);
    static bool isBuiltin(const bdld::DatumUdt& udt, int typeOffset);

    // Return which 'Builtin' is indicated by the specified 'udtData'. The
    // behavior is undefined unless 'udtData' is the '.data()' value of a
    // 'bdld::DatumUdt' whose '.type()' is 'UserDefinedTypes::e_BUILTIN' (after
    // considering any type offset).
    static Builtin fromUdtData(void* udtData);

    // Return a datum whose value corresponds to the specified 'builtin',
    // subject to the specified 'typeOffset' of the interpreter.
    static bdld::Datum toDatum(Builtin builtin, int typeOffset);

    // Return a reference to a static UTF-8 string contaning the canonical
    // spelling of the specified 'builtin'.
    static bsl::string_view name(Builtin builtin);
};

}  // namespace lspcore

#endif
