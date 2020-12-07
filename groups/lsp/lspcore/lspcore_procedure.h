#ifndef INCLUDED_LSPCORE_PROCEDURE
#define INCLUDED_LSPCORE_PROCEDURE

#include <bdld_datum.h>
#include <bsl_string.h>
#include <bsl_vector.h>
#include <bslma_usesbslmaallocator.h>
#include <bslmf_nestedtraitdeclaration.h>
#include <lspcore_userdefinedtypes.h>

namespace BloombergLP {
namespace bslma {
class Allocator;
}  // namespace bslma
}  // namespace BloombergLP

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

class Pair;

struct Procedure {
    // 'positionalParameters' and 'restParameters' contain the names of the
    // corresponding function parameters. Here are some examples:
    //
    // procedure definition    positional parameters    rest parameter
    // --------------------    ---------------------    --------------
    // (λ (a b . c) ...)       ["a", "b"]               "c"
    // (λ (a b) ...)           ["a", "b"]               ""
    // (λ a  ...)              []                       "a"
    //
    bsl::vector<bsl::string> positionalParameters;
    bsl::string              restParameter;  // or empty string

    // 'body' contains the body of the definition of the function, as an
    // s-expression (in particular, a list). For example, the procedure
    //
    //     (λ (x y)
    //       (debug "x:" x "y:" y)
    //       (if (> x y) x y))
    //
    // has a 'd_body' that is the list
    //
    //     ((debug "x:" x "y:" y) (if (> x y) x y))
    //
    const Pair* body;

    BSLMF_NESTED_TRAIT_DECLARATION(Procedure, bslma::UsesBslmaAllocator);

    explicit Procedure(bslma::Allocator* = 0);

    static bool isProcedure(const bdld::Datum&, int typeOffset);
    static bool isProcedure(const bdld::DatumUdt&, int typeOffset);

    // Note that 'Procedure' has no 'create' function. Procedures are created
    // by the 'Interpreter'.

    static const Procedure& access(const bdld::Datum&);
    static const Procedure& access(const bdld::DatumUdt&);
};

inline bool Procedure::isProcedure(const bdld::Datum& datum, int typeOffset) {
    return datum.isUdt() && isProcedure(datum.theUdt(), typeOffset);
}

inline bool Procedure::isProcedure(const bdld::DatumUdt& udt, int typeOffset) {
    return udt.type() - typeOffset == UserDefinedTypes::e_PROCEDURE;
}

inline const Procedure& Procedure::access(const bdld::Datum& datum) {
    BSLS_ASSERT(datum.isUdt());
    return access(datum.theUdt());
}

inline const Procedure& Procedure::access(const bdld::DatumUdt& udt) {
    return *static_cast<Procedure*>(udt.data());
}

}  // namespace lspcore

#endif
