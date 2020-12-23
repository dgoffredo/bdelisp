#include <bdlb_arrayutil.h>
#include <bsl_cstddef.h>
#include <bsl_queue.h>
#include <bsl_sstream.h>
#include <bsl_utility.h>
#include <lspcore_arithmeticutil.h>
#include <lspcore_builtinprocedures.h>
#include <lspcore_datumutil.h>
#include <lspcore_interpreter.h>
#include <lspcore_listutil.h>
#include <lspcore_nativeprocedureutil.h>
#include <lspcore_pair.h>
#include <lspcore_printutil.h>
#include <lspcore_procedure.h>
#include <lspcore_set.h>
#include <lspcore_symbolutil.h>

using namespace BloombergLP;

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

// helpers for 'BuiltinProcedures::equal'
namespace {

class NotEqual {
    const NativeProcedureUtil::Arguments* d_context_p;

  public:
    explicit NotEqual(const NativeProcedureUtil::Arguments& context)
    : d_context_p(&context) {
    }

    bool operator()(const bdld::Datum& left, const bdld::Datum& right) const {
#define TYPE_PAIR(LEFT, RIGHT) \
    ((bsl::uint64_t(LEFT) << 32) | bsl::uint64_t(RIGHT))

        switch (TYPE_PAIR(left.type(), right.type())) {
            // numeric cases
            case TYPE_PAIR(bdld::Datum::e_INTEGER, bdld::Datum::e_INTEGER64):
            case TYPE_PAIR(bdld::Datum::e_INTEGER64, bdld::Datum::e_INTEGER):
            case TYPE_PAIR(bdld::Datum::e_INTEGER, bdld::Datum::e_DOUBLE):
            case TYPE_PAIR(bdld::Datum::e_DOUBLE, bdld::Datum::e_INTEGER):
            case TYPE_PAIR(bdld::Datum::e_INTEGER, bdld::Datum::e_DECIMAL64):
            case TYPE_PAIR(bdld::Datum::e_DECIMAL64, bdld::Datum::e_INTEGER):
            case TYPE_PAIR(bdld::Datum::e_DOUBLE, bdld::Datum::e_DECIMAL64):
            case TYPE_PAIR(bdld::Datum::e_DECIMAL64, bdld::Datum::e_DOUBLE):

            case TYPE_PAIR(bdld::Datum::e_DECIMAL64, bdld::Datum::e_DECIMAL64):
            case TYPE_PAIR(bdld::Datum::e_DOUBLE, bdld::Datum::e_DOUBLE):
            case TYPE_PAIR(bdld::Datum::e_INTEGER, bdld::Datum::e_INTEGER):
            case TYPE_PAIR(bdld::Datum::e_INTEGER64, bdld::Datum::e_INTEGER64):

            case TYPE_PAIR(bdld::Datum::e_INTEGER64, bdld::Datum::e_DOUBLE):
            case TYPE_PAIR(bdld::Datum::e_DOUBLE, bdld::Datum::e_INTEGER64):
            case TYPE_PAIR(bdld::Datum::e_INTEGER64, bdld::Datum::e_DECIMAL64):
            case TYPE_PAIR(bdld::Datum::e_DECIMAL64,
                           bdld::Datum::e_INTEGER64): {
                // defer to '!ArithmeticUtil::equal(...)'
                bsl::vector<bdld::Datum> args;
                args.push_back(left);
                args.push_back(right);

                NativeProcedureUtil::Arguments subcontext = *d_context_p;
                subcontext.argsAndOutput                  = &args;

                ArithmeticUtil::equal(subcontext);
                return subcontext.argsAndOutput->front() ==
                       bdld::Datum::createBoolean(false);
            }
            // types for which the operator defined in `bdld::Datum` suffices
            case TYPE_PAIR(bdld::Datum::e_STRING, bdld::Datum::e_STRING):
            case TYPE_PAIR(bdld::Datum::e_BOOLEAN, bdld::Datum::e_BOOLEAN):
            case TYPE_PAIR(bdld::Datum::e_ERROR, bdld::Datum::e_ERROR):
            case TYPE_PAIR(bdld::Datum::e_DATE, bdld::Datum::e_DATE):
            case TYPE_PAIR(bdld::Datum::e_TIME, bdld::Datum::e_TIME):
            case TYPE_PAIR(bdld::Datum::e_DATETIME, bdld::Datum::e_DATETIME):
            case TYPE_PAIR(bdld::Datum::e_DATETIME_INTERVAL,
                           bdld::Datum::e_DATETIME_INTERVAL):
            case TYPE_PAIR(bdld::Datum::e_BINARY, bdld::Datum::e_BINARY):
                return left != right;
            // types that we drill down into
            case TYPE_PAIR(bdld::Datum::e_ARRAY, bdld::Datum::e_ARRAY):
                return (*this)(left.theArray(), right.theArray());
            case TYPE_PAIR(bdld::Datum::e_MAP, bdld::Datum::e_MAP):
                return mapsNotEqual(left.theMap(), right.theMap());
            case TYPE_PAIR(bdld::Datum::e_INT_MAP, bdld::Datum::e_INT_MAP):
                return mapsNotEqual(left.theIntMap(), right.theIntMap());
            case TYPE_PAIR(bdld::Datum::e_USERDEFINED,
                           bdld::Datum::e_USERDEFINED):
                return (*this)(left.theUdt(), right.theUdt());
            default:
                return true;
        }

#undef TYPE_PAIR
    }

    bool operator()(const bdld::DatumArrayRef& left,
                    const bdld::DatumArrayRef& right) const {
        bsl::size_t length = left.length();
        if (right.length() != length) {
            return true;
        }

        for (bsl::size_t i = 0; i < length; ++i) {
            if ((*this)(left[i], right[i])) {
                return true;
            }
        }

        return false;
    }

    template <typename MapRef>
    bool mapsNotEqual(const MapRef& left, const MapRef& right) const {
        const bsl::size_t size = left.size();
        if (right.size() != size) {
            return true;
        }

        for (bsl::size_t i = 0; i < size; ++i) {
            if (left[i].key() != right[i].key() ||
                (*this)(left[i].value(), right[i].value())) {
                return true;
            }
        }

        return false;
    }

    bool operator()(const bdld::DatumUdt& left,
                    const bdld::DatumUdt& right) const {
        if (left.type() != right.type()) {
            return true;
        }

        switch (left.type() - d_context_p->typeOffset) {
            case UserDefinedTypes::e_PAIR:
                return (*this)(Pair::access(left), Pair::access(right));
            case UserDefinedTypes::e_SYMBOL:
                // TODO: This is safe, but I still feel I messed up symbols.
                // Maybe what I need is to leave symbols as symbols, and then
                // have a separate 'ResolvedSymbol' type.
                return SymbolUtil::name(left) != SymbolUtil::name(right);
            case UserDefinedTypes::e_PROCEDURE:
            case UserDefinedTypes::e_NATIVE_PROCEDURE:
            case UserDefinedTypes::e_BUILTIN:
            default:
                return left.data() == right.data();
        }
    }

    bool operator()(const Pair& left, const Pair& right) const {
        // In general, 'left' and 'right' can be arbitrarily large binary
        // trees. Do a breadth-first traversal of them in tandem.
        bsl::queue<bsl::pair<const Pair*, const Pair*> > queue;
        queue.push(bsl::make_pair(&left, &right));

        do {
            const Pair& left  = *queue.front().first;
            const Pair& right = *queue.front().second;

            // This is a bit arcane, but the idea is that we're running the
            // same block of code twice: once for each Pair's '.first' and then
            // again for each Pair's '.second'. The data member to access is
            // abstracted out using a data member pointer, 'member'. So, this
            // code loops over the two cases: involving '.first' and then
            // involving '.second'.
            bdld::Datum Pair::*const members[] = { &Pair::first,
                                                   &Pair::second };
            for (bdld::Datum Pair::*const* memberIter = members;
                 memberIter != bdlb::ArrayUtil::end(members);
                 ++memberIter) {
                bdld::Datum Pair::*const member = *memberIter;

                if (Pair::isPair(left.*member, d_context_p->typeOffset)) {
                    if (Pair::isPair(right.*member, d_context_p->typeOffset)) {
                        queue.push(
                            bsl::make_pair(&Pair::access(left.*member),
                                           &Pair::access(right.*member)));
                    }
                    else {
                        return true;
                    }
                }
                else if ((*this)(left.first, right.first)) {
                    return true;
                }
            }

            queue.pop();
        } while (!queue.empty());

        return false;
    }
};

}  // namespace

void BuiltinProcedures::equal(const NativeProcedureUtil::Arguments& args) {
    bsl::vector<bdld::Datum>& argsVec = *args.argsAndOutput;

    // Per Scheme convention, (equal?) is true, as is (equal? one-arg).
    if (argsVec.size() <= 1) {
        argsVec.resize(1);
        argsVec.front() = bdld::Datum::createBoolean(true);
        return;
    }

    const bdld::Datum result = bdld::Datum::createBoolean(
        bsl::adjacent_find(argsVec.begin(), argsVec.end(), NotEqual(args)) ==
        argsVec.end());
    argsVec.resize(1);
    argsVec.front() = result;
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

void BuiltinProcedures::set(const NativeProcedureUtil::Arguments& args) {
    // Return a 'Set' of the arguments.
    const Set*            set = 0;
    const Set::Comparator before =
        DatumUtil::lessThanComparator(args.typeOffset);

    const bsl::vector<bdld::Datum> elements = *args.argsAndOutput;
    for (bsl::size_t i = 0; i < elements.size(); ++i) {
        set = Set::insert(set, elements[i], before, args.allocator);
    }

    const bdld::Datum result = Set::create(set, args.typeOffset);
    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = result;
}

void BuiltinProcedures::setContains(
    const NativeProcedureUtil::Arguments& args) {
    enforceArity("set-contains?", 2, args);

    const Set*            set   = Set::access((*args.argsAndOutput)[0]);
    const bdld::Datum&    value = (*args.argsAndOutput)[1];
    const Set::Comparator before =
        DatumUtil::lessThanComparator(args.typeOffset);

    const bdld::Datum result =
        bdld::Datum::createBoolean(Set::contains(set, value, before));
    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = result;
}

namespace {

void setInsertOrRemove(
    bsl::string_view name,
    const Set* (*function)(
        const Set*,
        const bdld::Datum&,
        const bsl::function<bool(const bdld::Datum&, const bdld::Datum&)>&,
        bslma::Allocator*),
    const NativeProcedureUtil::Arguments& args) {
    enforceArity(name, 2, args);

    const Set*            set   = Set::access((*args.argsAndOutput)[0]);
    const bdld::Datum&    value = (*args.argsAndOutput)[1];
    const Set::Comparator before =
        DatumUtil::lessThanComparator(args.typeOffset);

    set                      = function(set, value, before, args.allocator);
    const bdld::Datum result = Set::create(set, args.typeOffset);
    args.argsAndOutput->resize(1);
    args.argsAndOutput->front() = result;
}

}  // namespace

void BuiltinProcedures::setInsert(const NativeProcedureUtil::Arguments& args) {
    setInsertOrRemove("set-insert", &Set::insert, args);
}

void BuiltinProcedures::setRemove(const NativeProcedureUtil::Arguments& args) {
    setInsertOrRemove("set-remove", &Set::remove, args);
}

}  // namespace lspcore
