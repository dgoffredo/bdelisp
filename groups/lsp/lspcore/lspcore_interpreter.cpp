#include <bdld_datumarraybuilder.h>
#include <bdld_datumintmapbuilder.h>
#include <bdld_datummapowningkeysbuilder.h>
#include <bsl_cstddef.h>
#include <bsl_sstream.h>
#include <lspcore_builtins.h>
#include <lspcore_interpreter.h>
#include <lspcore_pair.h>
#include <lspcore_symbolutil.h>
#include <lspcore_userdefinedtypes.h>

namespace lspcore {
namespace {

void defineBuiltin(Environment& environment, Builtins::Builtin builtin) {
    environment.define(Builtins::name(builtin), Builtins::toDatum(builtin));
}

void defineBuiltins(Environment& environment) {
    defineBuiltin(environment, Builtins::e_LAMBDA);
    defineBuiltin(environment, Builtins::e_DEFINE);
    defineBuiltin(environment, Builtins::e_SET);
    defineBuiltin(environment, Builtins::e_QUOTE);

    // alternative spellings
    environment.define("lambda", Builtins::toDatum(Builtins::e_LAMBDA));
}

}  // namespace

Interpreter::Interpreter(int typeOffset, bslma::Allocator* allocator)
: d_globals(allocator)
, d_typeOffset(typeOffset) {
    defineBuiltins(d_globals);
}

bdld::Datum Interpreter::evaluate(const bdld::Datum& expression) {
    try {
        return evaluateExpression(expression, d_globals);
    }
    catch (const bdld::Datum& error) {
        return error;
    }
}

bdld::Datum Interpreter::evaluateExpression(const bdld::Datum& expression,
                                            Environment&       environment) {
    switch (expression.type()) {
        case bdld::Datum::e_NIL:
        case bdld::Datum::e_INTEGER:
        case bdld::Datum::e_DOUBLE:
        case bdld::Datum::e_STRING:
        case bdld::Datum::e_BOOLEAN:
        case bdld::Datum::e_ERROR:
        case bdld::Datum::e_DATE:
        case bdld::Datum::e_TIME:
        case bdld::Datum::e_DATETIME:
        case bdld::Datum::e_DATETIME_INTERVAL:
        case bdld::Datum::e_INTEGER64:
        case bdld::Datum::e_BINARY:
        case bdld::Datum::e_DECIMAL64:
            return expression;
        case bdld::Datum::e_USERDEFINED:
            switch (expression.theUdt().type() - d_typeOffset) {
                case UserDefinedTypes::e_PAIR:
                    return evaluatePair(Pair::access(expression), environment);
                case UserDefinedTypes::e_SYMBOL:
                    return evaluateSymbol(SymbolUtil::access(expression),
                                          environment);
                default:
                    return expression;
            }
        case bdld::Datum::e_ARRAY:
            if (expression.theArray().length() == 0) {
                return expression;
            }
            else {
                return evaluateArray(expression.theArray(), environment);
            }
        case bdld::Datum::e_MAP:
            return evaluateStringMap(expression.theMap(), environment);
        default:
            BSLS_ASSERT_OPT(expression.type() == bdld::Datum::e_INT_MAP);
            return evaluateIntMap(expression.theIntMap(), environment);
    }
}

void Interpreter::collectGarbage() {
    // TODO
}

bdld::Datum Interpreter::evaluateArray(const bdld::DatumArrayRef& array,
                                       Environment&) {
    BSLS_ASSERT(array.length() != 0);

    bdld::DatumArrayBuilder builder(allocator());
    for (bsl::size_t i = 0; i < array.length(); ++i) {
        builder.pushBack(evaluate(array[i]));
    }

    return builder.commit();
}

bdld::Datum Interpreter::evaluateStringMap(const bdld::DatumMapRef& map,
                                           Environment&) {
    bdld::DatumMapOwningKeysBuilder builder(allocator());
    for (bsl::size_t i = 0; i < map.size(); ++i) {
        builder.pushBack(map[i].key(), evaluate(map[i].value()));
    }

    return builder.commit();
}

bdld::Datum Interpreter::evaluateIntMap(const bdld::DatumIntMapRef& map,
                                        Environment&) {
    bdld::DatumIntMapBuilder builder(allocator());
    for (bsl::size_t i = 0; i < map.size(); ++i) {
        builder.pushBack(map[i].key(), evaluate(map[i].value()));
    }

    return builder.commit();
}

bdld::Datum Interpreter::evaluatePair(const Pair&, Environment&) {
    // TODO
    throw bdld::Datum::createError(
        -1, "list evaluation not implemented", allocator());
}

bdld::Datum Interpreter::evaluateSymbol(const bdld::Datum& asString,
                                        Environment&       environment) {
    const bsl::string_view name = asString.theString();

    const bsl::pair<const bsl::string, bdld::Datum>* const entry =
        environment.lookup(name);

    if (!entry) {
        bsl::ostringstream error;
        error << "unbound variable: " << name;
        throw bdld::Datum::createError(-1, error.str(), allocator());
    }

    if (entry->second == Builtins::toDatum(Builtins::e_UNDEFINED)) {
        // This can happen when a recursive binding form (e.g. 'letrec')
        // references variables prematurely, e.g.
        //
        //     (letrec ([foo bar] [bar "oops"]) ...)
        //
        // Even though 'bar' is visible to the definition of 'foo', but 'bar'
        // has not yet received a value, so this is still an error.
        bsl::ostringstream error;
        error << "variable referenced before it was defined: " << name;
        throw bdld::Datum::createError(-1, error.str(), allocator());
    }

    return entry->second;
}

bslma::Allocator* Interpreter::allocator() const {
    return d_globals.allocator();
}

}  // namespace lspcore
