#ifndef INCLUDED_LSPCORE_INTERPRETER
#define INCLUDED_LSPCORE_INTERPRETER

#include <bdld_datum.h>
#include <bsl_string_view.h>
#include <lspcore_environment.h>
#include <lspcore_nativeprocedureutil.h>

namespace BloombergLP {
namespace bslma {
class Allocator;
}  // namespace bslma
}  // namespace BloombergLP

namespace lspcore {
namespace bslma = BloombergLP::bslma;
namespace bdld  = BloombergLP::bdld;

class Pair;

class Interpreter {
    Environment d_globals;
    int         d_typeOffset;

  public:
    explicit Interpreter(int typeOffset, bslma::Allocator*);

    bdld::Datum evaluate(const bdld::Datum& expression);

    typedef NativeProcedureUtil::Signature NativeFunc;

    // Associate the specified 'name' in the global environment with the
    // specified 'function'. Return zero on success, or return a nonzero value
    // if 'name' is already in use within the global environment.
    int defineNativeProcedure(bsl::string_view name, NativeFunc* function);
    int defineNativeProcedure(bsl::string_view                 name,
                              const bsl::function<NativeFunc>& function);

    void collectGarbage();

  private:
    bdld::Datum evaluateExpression(const bdld::Datum&, Environment&);
    bdld::Datum evaluateArray(const bdld::DatumArrayRef&, Environment&);
    bdld::Datum evaluateStringMap(const bdld::DatumMapRef&, Environment&);
    bdld::Datum evaluateIntMap(const bdld::DatumIntMapRef&, Environment&);
    bdld::Datum evaluatePair(const Pair&, Environment&);
    bdld::Datum evaluateSymbol(const bdld::Datum&, Environment&);
    bdld::Datum evaluateLambda(const bdld::Datum&, Environment&);
    bdld::Datum evaluateDefine(const bdld::Datum&, Environment&);

    bdld::Datum invokeNative(const bdld::DatumUdt& nativeProcedure,
                             const bdld::Datum&    tail,
                             Environment&);

    bslma::Allocator* allocator() const;
};

}  // namespace lspcore

#endif
