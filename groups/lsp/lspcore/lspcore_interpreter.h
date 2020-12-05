#ifndef INCLUDED_LSPCORE_INTERPRETER
#define INCLUDED_LSPCORE_INTERPRETER

#include <bdld_datum.h>
#include <lspcore_environment.h>

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
    // TODO
    Environment d_globals;
    int         d_typeOffset;

  public:
    explicit Interpreter(int typeOffset, bslma::Allocator*);

    bdld::Datum evaluate(const bdld::Datum& expression);

    void collectGarbage();

  private:
    bdld::Datum evaluateExpression(const bdld::Datum&, Environment&);
    bdld::Datum evaluateArray(const bdld::DatumArrayRef&, Environment&);
    bdld::Datum evaluateStringMap(const bdld::DatumMapRef&, Environment&);
    bdld::Datum evaluateIntMap(const bdld::DatumIntMapRef&, Environment&);
    bdld::Datum evaluatePair(const Pair&, Environment&);
    bdld::Datum evaluateSymbol(const bdld::Datum&, Environment&);

    bslma::Allocator* allocator() const;
};

}  // namespace lspcore

#endif
