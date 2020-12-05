#ifndef INCLUDED_LSPCORE_PROCEDURE
#define INCLUDED_LSPCORE_PROCEDURE

#include <bsl_string.h>
#include <bsl_vector.h>
#include <lspcore_pair.h>

namespace lspcore {

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
    Pair body;
};

}  // namespace lspcore

#endif
