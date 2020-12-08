#include <bdld_datumarraybuilder.h>
#include <bdld_datumintmapbuilder.h>
#include <bdld_datummapowningkeysbuilder.h>
#include <bsl_cstddef.h>
#include <bsl_sstream.h>
#include <bsl_unordered_set.h>
#include <bslma_managedptr.h>
#include <lspcore_builtins.h>
#include <lspcore_interpreter.h>
#include <lspcore_listutil.h>
#include <lspcore_pair.h>
#include <lspcore_printutil.h>
#include <lspcore_procedure.h>
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
    defineBuiltin(environment, Builtins::e_IF, typeOffset);
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
    return !d_globals
                .define(name,
                        NativeProcedureUtil::create(
                            function, d_typeOffset, allocator()))
                .second;
}

int Interpreter::defineNativeProcedure(
    bsl::string_view name, const bsl::function<NativeFunc>& function) {
    return !d_globals
                .define(name,
                        NativeProcedureUtil::create(
                            function, d_typeOffset, allocator()))
                .second;
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

namespace {

// A form's 'Classification' tells the procedure evaluator whether it needs to
// treat the form specially when it appears in tail position.
enum Classification { e_OTHER, e_PROCEDURE_INVOCATION, e_IF };

Classification classify(const bdld::Datum& form,
                        const Environment& environment,
                        int                typeOffset) {
    if (!Pair::isPair(form, typeOffset)) {
        return e_OTHER;
    }

    const Pair& pair = Pair::access(form);
    if (!SymbolUtil::isSymbol(pair.first, typeOffset)) {
        return e_OTHER;
    }

    const bdld::Datum name = SymbolUtil::access(pair.first);
    const bsl::pair<const bsl::string, bdld::Datum>* const entry =
        environment.lookup(name.theString());
    // We could throw the "undefined identifier" exception here, but let's keep
    // it neat and let the caller redo that work.
    if (!entry) {
        return e_OTHER;
    }

    if (Builtins::isBuiltin(entry->second, typeOffset) &&
        Builtins::access(entry->second) == Builtins::e_IF) {
        return e_IF;
    }

    if (Procedure::isProcedure(entry->second, typeOffset)) {
        return e_PROCEDURE_INVOCATION;
    }

    return e_OTHER;
}

}  // namespace

bdld::Datum Interpreter::evaluatePair(const Pair&  pair,
                                      Environment& environment) {
    const bdld::Datum head = evaluateExpression(pair.first, environment);

    switch (head.type()) {
        case bdld::Datum::e_ARRAY:
        case bdld::Datum::e_MAP:
        case bdld::Datum::e_INT_MAP:
            // TODO: These are functions of their keys.
            throw bdld::Datum::createError(
                -1, "invoking dicts/vectors not yet implemented", allocator());
        case bdld::Datum::e_USERDEFINED: {
            const bdld::DatumUdt udt = head.theUdt();
            // Valid cases:
            // - procedure
            // - native procedure
            // - builtin
            //
            // Otherwise, fall through to the next section, which produces an
            // error.
            //
            if (Procedure::isProcedure(udt, d_typeOffset)) {
                return invokeProcedure(udt, pair.second, environment);
            }
            if (NativeProcedureUtil::isNativeProcedure(udt, d_typeOffset)) {
                // TODO: Could have it return a continuation in the future.
                return invokeNative(udt, pair.second, environment);
            }
            if (Builtins::isBuiltin(udt, d_typeOffset)) {
                switch (Builtins::fromUdtData(udt.data())) {
                    case Builtins::e_LAMBDA:
                        return evaluateLambda(pair.second, environment);
                    case Builtins::e_DEFINE:
                        return evaluateDefine(pair.second, environment);
                    case Builtins::e_SET:
                        // TODO
                        throw bdld::Datum::createError(
                            -1, "\"set!\" not yet implemented", allocator());
                    case Builtins::e_QUOTE:
                        // TODO
                        throw bdld::Datum::createError(
                            -1, "\"quote\" not yet implemented", allocator());
                    case Builtins::e_IF:
                        // TODO
                        throw bdld::Datum::createError(
                            -1, "\"if\" not yet implemented", allocator());
                    case Builtins::e_UNDEFINED:
                        break;
                }
            }
        }
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

bdld::Datum Interpreter::evaluateLambda(const bdld::Datum& tail,
                                        Environment&       environment) {
    // 'tail' is the lambda expression but without the leading 'lambda' or 'λ'
    // symbol. The form, in a syntax-rules like notation, is one of:
    //
    //     (param-list body-first body-rest ...)
    //     ((param ...) body-first body-rest ...)
    //     ((param params ... . rest-param) body-first body-rest ...)
    //
    // That is, 'tail' is a list with at least two elements.
    //
    // The first element is the parameters, and is one of: a symbol, a list of
    // symbols, or an improper list of symbols (where the 'rest-param' is bound
    // to a list).
    //
    // The second element is the first form in the body of the procedure, and
    // may be followed by zero or more additional forms.

    // First do some validation of the overall shape of the lambda form.
    if (!Pair::isPair(tail, d_typeOffset)) {
        throw bdld::Datum::createError(
            -1,
            "λ expression must be a list containing parameters and a body",
            allocator());
    }

    const Pair&        tailPair   = Pair::access(tail);
    const bdld::Datum& parameters = tailPair.first;
    const bdld::Datum& body       = tailPair.second;
    if (body.isNull()) {
        throw bdld::Datum::createError(
            -1, "λ expression body must be nonempty", allocator());
    }
    if (!Pair::isPair(body, d_typeOffset)) {
        throw bdld::Datum::createError(
            -1, "λ expression must be a proper list", allocator());
    }

    // 'parameterNames' is used to avoid duplicates
    bsl::unordered_set<bsl::string> parameterNames;

    // Use a 'ManagedPtr' so that if we throw an exception, the
    // under-construction 'Procedure' is freed. This isn't necessary, since we
    // will be using some form of garbage collection, but might as well
    // generate less garbage when we can.
    bslma::ManagedPtr<Procedure> procedure(
        new (*allocator()) Procedure(allocator()), allocator());

    // Parse the parameters.
    if (SymbolUtil::isSymbol(parameters, d_typeOffset)) {
        procedure->restParameter = SymbolUtil::access(parameters).theString();
    }
    else if (parameters.isNull()) {
        // no positional parameters, and no rest parameter
    }
    else if (Pair::isPair(parameters, d_typeOffset)) {
        bdld::Datum rest = parameters;
        do {
            const Pair&        pair      = Pair::access(rest);
            const bdld::Datum& parameter = pair.first;

            if (!SymbolUtil::isSymbol(parameter, d_typeOffset)) {
                throw bdld::Datum::createError(
                    -1,
                    "λ expression parameters must all be symbols",
                    allocator());
            }
            const bdld::Datum param = SymbolUtil::access(parameter);
            if (!parameterNames.insert(param.theString()).second) {
                bsl::ostringstream error;
                error << "duplicate procedure parameter name: "
                      << param.theString();
                throw bdld::Datum::createError(-1, error.str(), allocator());
            }
            procedure->positionalParameters.push_back(param.theString());

            rest = pair.second;
        } while (Pair::isPair(rest, d_typeOffset));

        // If 'rest' isn't null by this point, then there's a trailing "rest"
        // parameter, e.g.
        //
        //     (λ (foo bar . rest) ...)
        //
        if (!rest.isNull()) {
            if (!SymbolUtil::isSymbol(rest, d_typeOffset)) {
                throw bdld::Datum::createError(
                    -1,
                    "λ expression parameters must all be symbols",
                    allocator());
            }

            const bdld::Datum param = SymbolUtil::access(rest);
            if (!parameterNames.insert(param.theString()).second) {
                bsl::ostringstream error;
                error << "duplicate procedure parameter name: "
                      << param.theString();
                throw bdld::Datum::createError(-1, error.str(), allocator());
            }
            procedure->restParameter = param.theString();
        }
    }
    else {
        throw bdld::Datum::createError(
            -1,
            "λ expression's second element must be a proper list of symbols, "
            "an improper list of symbols, or a symbol",
            allocator());
    }

    // and finally the body
    if (!ListUtil::isProperList(body, d_typeOffset)) {
        throw bdld::Datum::createError(
            -1, "λ expression body must be a proper list", allocator());
    }
    procedure->body        = &Pair::access(body);
    procedure->environment = &environment;
    environment.markAsReferenced();

    return bdld::Datum::createUdt(
        procedure.release().first,  // the pointer
        UserDefinedTypes::e_PROCEDURE + d_typeOffset);
}

bdld::Datum Interpreter::evaluateDefine(const bdld::Datum& tail,
                                        Environment&       environment) {
    // Other lisps disallow 'define' in "expression context." When I start
    // thinking more carefully about environments and macros, I'll probably do
    // the same. For now, though, 'define' can appear anywhere, and evaluates
    // to the defined value.

    // 'tail' is the part of the form following the "define" symbol.
    //
    //     (name value)
    //
    // where 'name' is a symbol.
    //
    // TODO: add support to the lambda shorthand, i.e.
    //
    //     ((name args ...) body rest ...)
    //
    // TODO: currently not supported.

    if (!Pair::isPair(tail, d_typeOffset)) {
        throw bdld::Datum::createError(
            -1,
            "\"define\" form must be a proper list having three elements",
            allocator());
    }

    const Pair& form = Pair::access(tail);
    bdld::Datum name = form.first;
    if (!SymbolUtil::isSymbol(name, d_typeOffset)) {
        throw bdld::Datum::createError(-1,
                                       "\"define\" form's first argument must "
                                       "be a symbol naming the value",
                                       allocator());
    }
    name = SymbolUtil::access(name);

    if (!Pair::isPair(form.second, d_typeOffset)) {
        throw bdld::Datum::createError(
            -1,
            "\"define\" form must be a proper list having three elements",
            allocator());
    }

    const Pair& rest = Pair::access(form.second);
    if (!rest.second.isNull()) {
        throw bdld::Datum::createError(
            -1,
            "\"define\" form must be a proper list having three elements",
            allocator());
    }

    bsl::pair<bsl::pair<const bsl::string, bdld::Datum>*, bool> entry =
        environment.define(
            name.theString(),
            Builtins::toDatum(Builtins::e_UNDEFINED, d_typeOffset));

    const bdld::Datum value = evaluateExpression(rest.first, environment);
    entry.first->second     = value;
    return value;
}

bdld::Datum Interpreter::invokeProcedure(const bdld::DatumUdt& procedure,
                                         const bdld::Datum&    tail,
                                         Environment&          environment) {
    BSLS_ASSERT(Procedure::isProcedure(procedure, d_typeOffset));

    // 'argsEnv' is the environment in which the procedure's arguments are
    // evaluated.
    Environment* argsEnv = &environment;
    // 'env' is the environment in which the body of the procedure is
    // evaluated. Note that this might be the same as 'argsEnv', for a
    // recursive tail call where no lambdas were generated. Better escape
    // analysis might be added in the future.
    const Procedure* proc = &Procedure::access(procedure);
    Environment*     env =
        new (*allocator()) Environment(proc->environment, allocator());
    bdld::Datum restArgs = tail;
    // TODO: change calling convention so stack can be shared in interpreter
    // (fewer allocations)
    bsl::vector<bdld::Datum> argStack;
// We jump back to 'tailCall' when there's an invocation in tail position.
// Before 'goto', the code will set up 'argsEnv', 'env', 'proc', 'restArgs',
// and 'argStack' appropriately so that it's as if we called 'invokeProcedure'
// again.
tailCall:
    if (!restArgs.isNull() && !Pair::isPair(restArgs, d_typeOffset)) {
        throw bdld::Datum::createError(
            -1,
            "procedure invocation form must be a proper list",
            allocator());
    }

    // Evaluate the procedure's arguments, placing them into 'argStack'. Once
    // we've done that, we'll associate them with their names in 'env'.
    int numArgs = 0;
    for (bsl::vector<bsl::string>::const_iterator iter =
             proc->positionalParameters.begin();
         iter != proc->positionalParameters.end();
         ++iter) {
        if (restArgs.isNull()) {
            throw bdld::Datum::createError(
                -1, "not enough arguments passed to procedure", allocator());
        }
        if (!Pair::isPair(restArgs, d_typeOffset)) {
            throw bdld::Datum::createError(
                -1,
                "procedure invocation form must be a proper list",
                allocator());
        }

        const Pair& pair = Pair::access(restArgs);
        argStack.push_back(evaluateExpression(pair.first, *argsEnv));
        ++numArgs;
        restArgs = pair.second;
    }

    // If the procedure has a 'rest' argument, e.g. "baz" in '(λ (foo bar .
    // baz) ...)', then map all remaining 'restArgs' into a list of evaluated
    // arguments and push the resulting list onto 'argStack'.
    // We'll use 'argStack' temporarily to store the evaluated arguments before
    // converting them into a list.
    if (!proc->restParameter.empty()) {
        const int restBeginIndex = numArgs;
        while (Pair::isPair(restArgs, d_typeOffset)) {
            const Pair& pair = Pair::access(restArgs);
            argStack.push_back(evaluateExpression(pair.first, *argsEnv));
            restArgs = pair.second;
        }

        if (!restArgs.isNull()) {
            throw bdld::Datum::createError(
                -1,
                "procedure invocation form must be a proper list",
                allocator());
        }

        const bdld::Datum restList =
            ListUtil::createList(argStack.begin() + restBeginIndex,
                                 argStack.end(),
                                 d_typeOffset,
                                 allocator());
        // Note: The following three lines (commented out) are not necessary.
        // We can instead put 'restList' directly into the procedure's
        // environment, now that we're done evaluating arguments.
        //
        //     argStack.erase(argStack.begin() + restBeginIndex,
        //     argStack.end()); argStack.push_back(restList);
        //     ++numArgs;
        env->defineOrRedefine(proc->restParameter, restList);
    }

    // Now bind the evaluated arguments from 'argStack' into the procedure's
    // environment ('env').
    for (bsl::size_t i = 0; i < proc->positionalParameters.size(); ++i) {
        // e.g. if the third positional parameter is named "foo", bind the
        // third evaluated argument from 'argStack' to the name "foo" in 'env'.
        env->defineOrRedefine(proc->positionalParameters[i], argStack[i]);
    }

    // Evaluate each of the forms in 'proc->body'. Discard all results except
    // for the last one.
    //
    // The last form is treated specially: if it's an 'if' form or a procedure
    // invocation, then we defer evaluation in order to handle tail calls
    // properly. If it's some other kind of form, then we just return the
    // result of evaluating it.
    const Pair* rest = proc->body;
    // while we're not at the last form...
    while (!rest->second.isNull()) {
        (void)evaluateExpression(rest->first, *env);
        // We can assume that 'rest.second' is a 'Pair', because 'env.body' is
        // guarnateed by the 'lambda' evaluator to be a proper list.
        rest = &Pair::access(rest->second);
    }

    // Now we're on the last form. Classify it as one of the following:
    // - an 'if' statement
    // - a procedure invocation
    // - other
    //
    // If it's "other," then just evaluate it and return the value.
    //
    // If it's a procedure invocation, then reset the variables in this
    // function and 'goto tailCall'.
    //
    // If it's an 'if' statement, then evaluate the predicate to decide which
    // subform to evaluate. Then go around the following loop again. The idea
    // is to traverse nested 'if' expressions that are in tail position, in
    // order to get to the underlying value or tail call.
    for (bdld::Datum form = rest->first;;) {
        Classification classification =
            classify(form, environment, d_typeOffset);
        switch (classification) {
            case e_OTHER:
                return evaluateExpression(form, *env);
            case e_IF: {
                // An 'if' form has three arguments:
                //
                //     (if <predicate> <then> <else>)
                //
                // Evaluate the <predicate>. If it's '#f', then loop around
                // using <else> as the 'form'. Otherwise, loop around using
                // <then> as the form.
                const Pair* ifTail = &Pair::access(form);
                // TODO: refactor all of this 'if' form unpacking into a
                // function that this can share with the non-tail-position 'if'
                // handler.
                if (!Pair::isPair(ifTail->second, d_typeOffset)) {
                    throw bdld::Datum::createError(
                        -1,
                        "\"if\" form must have three arguments: <predicate> "
                        "<then> <else>",
                        allocator());
                }
                ifTail                       = &Pair::access(ifTail->second);
                const bdld::Datum& predicate = ifTail->first;

                if (!Pair::isPair(ifTail->second, d_typeOffset)) {
                    throw bdld::Datum::createError(
                        -1,
                        "\"if\" form must have three arguments: <predicate> "
                        "<then> <else>",
                        allocator());
                }
                ifTail                      = &Pair::access(ifTail->second);
                const bdld::Datum& thenForm = ifTail->first;

                if (!Pair::isPair(ifTail->second, d_typeOffset)) {
                    throw bdld::Datum::createError(
                        -1,
                        "\"if\" form must have three arguments: <predicate> "
                        "<then> <else>",
                        allocator());
                }
                ifTail                      = &Pair::access(ifTail->second);
                const bdld::Datum& elseForm = ifTail->first;
                if (!ifTail->second.isNull()) {
                    throw bdld::Datum::createError(
                        -1, "\"if\" form must be a proper list", allocator());
                }

                const bdld::Datum predicateResult =
                    evaluateExpression(predicate, *env);
                if (predicateResult == bdld::Datum::createBoolean(false)) {
                    form = elseForm;
                }
                else {
                    form = thenForm;
                }
                break;  // or, equivalently, 'continue'
            }
            default: {
                BSLS_ASSERT(classification == e_PROCEDURE_INVOCATION);
                // Here are the variables that we need to set up before 'goto
                // tailCall':
                //
                // - 'Environment* argsEnv'
                // - 'Environment* env'
                // - 'const Procedure* proc'
                // - 'bdld::Datum restArgs'
                // - 'bsl::vector<bdld::Datum> argStack'
                //
                // Depending on whether 'env->wasReferenced()', we might be
                // able to reuse 'env' as both 'argsEnv' and 'env'. Otherwise,
                // 'env' will have to be a fresh 'Environment'.
                //
                // 'proc' will be the procedure that we're calling (extracted
                // from 'form').
                //
                // 'restArgs' will be the tail of 'form'.
                //
                // 'argStack' will be cleared.
                //
                const Pair& invocation = Pair::access(form);
                proc                   = &Procedure::access(
                    evaluateExpression(invocation.first, *env));
                restArgs = invocation.second;
                argsEnv  = env;
                if (env->wasReferenced()) {
                    env = new (*allocator()) Environment(env, allocator());
                }
                argStack.clear();
                goto tailCall;
            }
        }
    }
}

bdld::Datum Interpreter::invokeNative(const bdld::DatumUdt& nativeProcedure,
                                      const bdld::Datum&    tail,
                                      Environment&          environment) {
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
