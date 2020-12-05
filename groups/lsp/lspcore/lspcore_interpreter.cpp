#include <bdld_datumarraybuilder.h>
#include <bdld_datumintmapbuilder.h>
#include <bdld_datummapowningkeysbuilder.h>
#include <bsl_cstddef.h>
#include <bsl_sstream.h>
#include <lspcore_builtins.h>
#include <lspcore_interpreter.h>
#include <lspcore_pair.h>
#include <lspcore_printutil.h>
#include <lspcore_symbolutil.h>
#include <lspcore_userdefinedtypes.h>

namespace lspcore {
namespace {

void defineBuiltin(Environment&      environment,
                   Builtins::Builtin builtin,
                   int               typeOffset) {
    environment.define(Builtins::name(builtin),
                       Builtins::toDatum(builtin, typeOffset));
}

void defineDefaultGlobalEnvironment(Environment& environment, int typeOffset) {
    defineBuiltin(environment, Builtins::e_LAMBDA, typeOffset);
    defineBuiltin(environment, Builtins::e_DEFINE, typeOffset);
    defineBuiltin(environment, Builtins::e_SET, typeOffset);
    defineBuiltin(environment, Builtins::e_QUOTE, typeOffset);

    // alternative spellings
    environment.define("lambda",
                       Builtins::toDatum(Builtins::e_LAMBDA, typeOffset));
}

}  // namespace

Interpreter::Interpreter(int typeOffset, bslma::Allocator* allocator)
: d_globals(allocator)
, d_typeOffset(typeOffset) {
    defineDefaultGlobalEnvironment(d_globals, d_typeOffset);
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

int Interpreter::defineNativeProcedure(bsl::string_view name,
                                       NativeFunc*      function) {
    return !d_globals.define(
        name,
        NativeProcedureUtil::create(function, d_typeOffset, allocator()));
}

int Interpreter::defineNativeProcedure(
    bsl::string_view name, const bsl::function<NativeFunc>& function) {
    return !d_globals.define(
        name,
        NativeProcedureUtil::create(function, d_typeOffset, allocator()));
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

bdld::Datum Interpreter::evaluatePair(const Pair&  pair,
                                      Environment& environment) {
    const bdld::Datum head = evaluateExpression(pair.first, environment);
    // Cases:
    // - builtin: dispatch to appropriate method
    //     - quote
    //     - set!
    //     - lambda
    // - native_procedure: bsl::vector<Datum> of the evaluated args, call
    // function
    // - procedure: inspect the procedure to get positional and tail args
    //     - positionals go into a bsl::vector<Datum>, while tail args cons
    //     into a list
    //     - for loop, goto, all that jazz. This will be a large function.
    // - otherwise, error: cannot invoke object as procedure ...
    //     - WAIT, what about applying keys to maps and stuff. Fine, fine...
    //     - a bunch of special cases to check
    // TODO

    switch (head.type()) {
        case bdld::Datum::e_ARRAY:
        case bdld::Datum::e_MAP:
        case bdld::Datum::e_INT_MAP:
            // TODO: These are functions of their keys.
            throw bdld::Datum::createError(
                -1, "invoking dicts/vectors not yet implemented", allocator());
        case bdld::Datum::e_USERDEFINED:
            // Valid cases:
            // - procedure
            // - native procedure
            // - builtin
            // Otherwise, fall through.
            if (NativeProcedureUtil::isNativeProcedure(head, d_typeOffset)) {
                // TODO: Could have it return a continuation in the future.
                return invokeNative(head, pair.second, environment);
            }
            // TODO: handle other valid cases

            // fall through
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
        case bdld::Datum::e_BINARY:
        case bdld::Datum::e_DECIMAL64:
        case bdld::Datum::e_INTEGER64: {
            bsl::ostringstream error;
            error << "cannot be invoked as a procedure: ";
            PrintUtil::print(error, head, d_typeOffset);
            throw bdld::Datum::createError(-1, error.str(), allocator());
        }
    }

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

    if (entry->second ==
        Builtins::toDatum(Builtins::e_UNDEFINED, d_typeOffset)) {
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

bdld::Datum Interpreter::invokeNative(const bdld::Datum& nativeProcedure,
                                      const bdld::Datum& tail,
                                      Environment&       environment) {
    BSLS_ASSERT(
        NativeProcedureUtil::isNativeProcedure(nativeProcedure, d_typeOffset));

    // Evaluate the arguments. It's an error if 'tail' is not a proper list.
    bsl::vector<bdld::Datum> argumentsAndResult;
    bdld::Datum              rest = tail;
    while (Pair::isPair(rest, d_typeOffset)) {
        const Pair& pair = Pair::access(rest);
        argumentsAndResult.push_back(
            evaluateExpression(pair.first, environment));
        rest = pair.second;
    }

    if (!rest.isNull()) {
        bsl::ostringstream error;
        error << "procedure invocation arguments is an improper list: ";
        PrintUtil::print(error, tail, d_typeOffset);
        throw bdld::Datum::createError(-1, error.str(), allocator());
    }

    NativeProcedureUtil::invoke(
        nativeProcedure, argumentsAndResult, environment, allocator());
    // The contract with native procedures states that they deliver a result by
    // resizing the argument vector and assigning to its sole element.
    BSLS_ASSERT_OPT(argumentsAndResult.size() == 1);
    return argumentsAndResult[0];
}

bslma::Allocator* Interpreter::allocator() const {
    return d_globals.allocator();
}

}  // namespace lspcore
