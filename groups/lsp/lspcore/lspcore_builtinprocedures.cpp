#include <bsl_cstddef.h>
#include <bsl_sstream.h>
#include <lspcore_builtinprocedures.h>
#include <lspcore_interpreter.h>
#include <lspcore_listutil.h>
#include <lspcore_nativeprocedureutil.h>
#include <lspcore_pair.h>
#include <lspcore_printutil.h>
#include <lspcore_procedure.h>

namespace lspcore {
namespace {

void enforceArity(bsl::string_view                      name,
                  bsl::size_t                           arity,
                  const NativeProcedureUtil::Arguments& args) {
    if (args.argsAndOutput->size() == arity) {
        return;  // all good
    }

    bsl::ostringstream error;
    error << "procedure \"" << name << "\" takes " << arity
          << " arguments, but was invoked with " << args.argsAndOutput->size();
    throw bdld::Datum::createError(-1, error.str(), args.allocator);
}

}  // namespace

void BuiltinProcedures::isPair(const NativeProcedureUtil::Arguments& args) {
    enforceArity("pair?", 1, args);

    const bdld::Datum result = bdld::Datum::createBoolean(
        Pair::isPair(args.argsAndOutput->front(), args.typeOffset));
    args.argsAndOutput->front() = result;
}

void BuiltinProcedures::pair(const NativeProcedureUtil::Arguments& args) {
    enforceArity("pair", 2, args);

    const bdld::Datum& first  = (*args.argsAndOutput)[0];
    const bdld::Datum& second = (*args.argsAndOutput)[1];
    const bdld::Datum  result =
        Pair::create(first, second, args.typeOffset, args.allocator);
    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = result;
}

void BuiltinProcedures::pairFirst(const NativeProcedureUtil::Arguments& args) {
    enforceArity("pair-first", 1, args);

    const bdld::Datum& arg = args.argsAndOutput->front();
    if (!Pair::isPair(arg, args.typeOffset)) {
        throw bdld::Datum::createError(
            -1, "argument to \"pair-first\" must be a pair", args.allocator);
    }

    const bdld::Datum result = Pair::access(arg).first;
    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = result;
}

void BuiltinProcedures::pairSecond(
    const NativeProcedureUtil::Arguments& args) {
    enforceArity("pair-second", 1, args);

    const bdld::Datum& arg = args.argsAndOutput->front();
    if (!Pair::isPair(arg, args.typeOffset)) {
        throw bdld::Datum::createError(
            -1, "argument to \"pair-second\" must be a pair", args.allocator);
    }

    const bdld::Datum result = Pair::access(arg).second;
    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = result;
}

void BuiltinProcedures::isNull(const NativeProcedureUtil::Arguments& args) {
    enforceArity("null?", 1, args);

    const bdld::Datum result =
        bdld::Datum::createBoolean(args.argsAndOutput->front().isNull());
    args.argsAndOutput->front() = result;
}

void BuiltinProcedures::equal(const NativeProcedureUtil::Arguments& args) {
    // TODO
    throw bdld::Datum::createError(
        -1, "\"equal?\" is not yet implemented", args.allocator);
}

void BuiltinProcedures::list(const NativeProcedureUtil::Arguments& args) {
    const bdld::Datum result = ListUtil::createList(
        *args.argsAndOutput, args.typeOffset, args.allocator);
    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = result;
}

void BuiltinProcedures::apply(const NativeProcedureUtil::Arguments& args) {
    // 'apply' takes two arguments:
    // 1. a procedure or native procedure to invoke
    // 2. a list of data to use as the arguments to the procedure
    enforceArity("apply", 2, args);

    const bdld::Datum& procedure = (*args.argsAndOutput)[0];
    if (!(Procedure::isProcedure(procedure, args.typeOffset) ||
          NativeProcedureUtil::isNativeProcedure(procedure,
                                                 args.typeOffset))) {
        throw bdld::Datum::createError(
            -1,
            "first argument to \"apply\" must be a procedure",
            args.allocator);
    }

    const bdld::Datum& argList = (*args.argsAndOutput)[1];
    if (!ListUtil::isProperList(argList, args.typeOffset)) {
        throw bdld::Datum::createError(
            -1,
            "second argument to \"apply\" must be a proper list",
            args.allocator);
    }

    const bdld::Datum invocation =
        Pair::create(procedure, argList, args.typeOffset, args.allocator);
    const bdld::Datum result =
        args.interpreter->evaluateExpression(invocation, *args.environment);

    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = result;
}

void BuiltinProcedures::raise(const NativeProcedureUtil::Arguments& args) {
    enforceArity("raise", 1, args);

    throw args.argsAndOutput->front();
}

}  // namespace lspcore
