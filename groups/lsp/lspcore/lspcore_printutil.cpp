#include <baljsn_printutil.h>
#include <balxml_typesprintutil.h>
#include <bdld_datum.h>
#include <bdld_datumbinaryref.h>
#include <bdld_datumerror.h>
#include <bdld_datumudt.h>
#include <bdlde_base64encoder.h>
#include <bdldfp_decimal.h>
#include <bdlt_date.h>
#include <bdlt_datetime.h>
#include <bdlt_datetimeinterval.h>
#include <bdlt_intervalconversionutil.h>
#include <bdlt_iso8601util.h>
#include <bdlt_time.h>
#include <bsl_ostream.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bsls_assert.h>
#include <lspcore_builtins.h>
#include <lspcore_pair.h>
#include <lspcore_printutil.h>
#include <lspcore_procedure.h>
#include <lspcore_set.h>
#include <lspcore_symbolutil.h>
#include <lspcore_userdefinedtypes.h>

using namespace BloombergLP;

namespace lspcore {
namespace {

// flags that can be bitwise or'd together and passed to 'printList':
const int k_NO_PARENTHESES = 1;

struct PrintVisitor {
    bsl::ostream&                 stream;
    int                           typeOffset;
    bsl::vector<const Procedure*> procedureStack;

    explicit PrintVisitor(bsl::ostream& stream, int typeOffset)
    : stream(stream)
    , typeOffset(typeOffset) {
    }

    void operator()(bslmf::Nil);
    void operator()(const bdlt::Date&);
    void operator()(const bdlt::Datetime&);
    void operator()(const bdlt::DatetimeInterval&);
    void operator()(const bdlt::Time&);
    void operator()(const bsl::string_view&);
    void operator()(bool);
    void operator()(bsls::Types::Int64);
    void operator()(double);
    void operator()(const bdld::DatumError&);
    void operator()(int);
    void operator()(const bdld::DatumUdt&);
    void operator()(const bdld::DatumArrayRef&);
    void operator()(const bdld::DatumIntMapRef&);
    void operator()(const bdld::DatumMapRef&);
    void operator()(const bdld::DatumBinaryRef&);
    void operator()(bdldfp::Decimal64);
    void printList(const Pair&, int flags = 0);
    void printSymbol(const bdld::DatumUdt&);
    template <typename MapRef, typename Entry>
    void printMap(const MapRef&);
    void printPointer(const void*);
    void printProcedure(const Procedure&);
    void printSet(const Set*);
};

void PrintVisitor::operator()(bslmf::Nil) {
    stream << "()";
}

void PrintVisitor::operator()(const bdlt::Date& value) {
    bdlt::Iso8601Util::generate(stream, value);
}

void PrintVisitor::operator()(const bdlt::Datetime& value) {
    bdlt::Iso8601Util::generate(stream, value);
}

void PrintVisitor::operator()(const bdlt::DatetimeInterval& value) {
    // datetime intervals are prefixed by a "#" in this lisp
    stream << "#";
    bdlt::Iso8601Util::generate(
        stream, bdlt::IntervalConversionUtil::convertToTimeInterval(value));
}

void PrintVisitor::operator()(const bdlt::Time& value) {
    bdlt::Iso8601Util::generate(stream, value);
}

void PrintVisitor::operator()(const bsl::string_view& value) {
    baljsn::PrintUtil::printValue(stream, value);
}

void PrintVisitor::operator()(bool value) {
    stream << (value ? "#t" : "#f");
}

void PrintVisitor::operator()(bsls::Types::Int64 value) {
    baljsn::PrintUtil::printValue(stream, value);
    stream << "L";
}

void PrintVisitor::operator()(double value) {
    baljsn::PrintUtil::printValue(stream, value);
    stream << "B";
}

void PrintVisitor::operator()(const bdld::DatumError& error) {
    // e.g. #error[42] or #error[42 "bad thing happened"]
    stream << "#error[" << error.code();
    if (!error.message().empty()) {
        stream << " ";
        (*this)(error.message());
    }
    stream << "]";
}

void PrintVisitor::operator()(int value) {
    baljsn::PrintUtil::printValue(stream, value);
}

void PrintVisitor::operator()(const bdld::DatumUdt& value) {
    switch (value.type() - typeOffset) {
        case UserDefinedTypes::e_PAIR:
            return printList(Pair::access(value));
        case UserDefinedTypes::e_SYMBOL:
            return printSymbol(value);
        case UserDefinedTypes::e_BUILTIN:
            // can't user 'printSymbol' because there's no 'bdld::Datum'
            // string. That's fine, because we don't need it -- builtin names
            // are hard-coded.
            stream << Builtins::name(Builtins::fromUdtData(value.data()));
            return;
        case UserDefinedTypes::e_PROCEDURE:
            printProcedure(Procedure::access(value));
            return;
        case UserDefinedTypes::e_NATIVE_PROCEDURE:
            stream << "#procedure[native ";
            printPointer(value.data());
            stream << "]";
            return;
        case UserDefinedTypes::e_SET:
            printSet(Set::access(value));
            return;
    }

    // Otherwise, it's some user-defined type we're not aware of. Print its
    // type ID and its address, e.g.: '#udt[23 "0x3434324"]'.
    stream << "#udt[" << value.type() << " \"";
    printPointer(value.data());
    stream << "\"]";
}

void PrintVisitor::operator()(const bdld::DatumArrayRef& array) {
    stream << "[";
    typedef bdld::Datum Entry;
    const Entry*        iter = array.data();
    const Entry* const  end  = iter + array.length();
    if (iter != end) {
        iter->apply(*this);
        for (++iter; iter != end; ++iter) {
            stream << " ";
            iter->apply(*this);
        }
    }
    stream << "]";
}

template <typename MapRef, typename Entry>
void PrintVisitor::printMap(const MapRef& map) {
    stream << "{";
    const Entry*       iter = map.data();
    const Entry* const end  = iter + map.size();
    if (iter != end) {
        (*this)(iter->key());
        stream << " ";
        iter->value().apply(*this);
        for (++iter; iter != end; ++iter) {
            stream << " ";
            (*this)(iter->key());
            stream << " ";
            iter->value().apply(*this);
        }
    }
    stream << "}";
}

void PrintVisitor::operator()(const bdld::DatumIntMapRef& map) {
    return printMap<bdld::DatumIntMapRef, bdld::DatumIntMapEntry>(map);
}

void PrintVisitor::operator()(const bdld::DatumMapRef& map) {
    return printMap<bdld::DatumMapRef, bdld::DatumMapEntry>(map);
}

void PrintVisitor::operator()(const bdld::DatumBinaryRef& value) {
    stream << "#base64\"";
    balxml::TypesPrintUtil::printBase64(
        stream,
        bsl::string_view(static_cast<const char*>(value.data()),
                         value.size()));
    stream << "\"";
}

void PrintVisitor::operator()(bdldfp::Decimal64 value) {
    baljsn::PrintUtil::printValue(stream, value);
}

void PrintVisitor::printList(const Pair& pair, int flags) {
    if (!(flags & k_NO_PARENTHESES)) {
        stream << "(";
    }
    pair.first.apply(*this);
    bdld::Datum item = pair.second;

    while (!item.isNull()) {
        if (Pair::isPair(item, typeOffset)) {
            const Pair& pair = Pair::access(item);
            stream << " ";
            pair.first.apply(*this);
            item = pair.second;
        }
        else {
            stream << " . ";
            item.apply(*this);
            break;
        }
    }

    if (!(flags & k_NO_PARENTHESES)) {
        stream << ")";
    }
}

void PrintVisitor::printSymbol(const bdld::DatumUdt& value) {
    BSLS_ASSERT(SymbolUtil::isSymbol(value, typeOffset));

    if (!procedureStack.empty()) {
        stream << SymbolUtil::name(value, *procedureStack.back()).theString();
    }
    else {
        stream << SymbolUtil::name(value).theString();
    }
}

void PrintVisitor::printPointer(const void* value) {
    if (value) {
        stream << value;
    }
    else {
        stream << "0x0";
    }
}

void PrintVisitor::printProcedure(const Procedure& value) {
    BSLS_ASSERT(value.body);

    stream << "#procedure[(λ ";

    // Example parameters forms:
    //
    //     ()
    //     foo
    //     (foo)
    //     (foo bar)
    //     (foo . bar)
    //     (foo bar . baz)
    //
    if (value.positionalParameters.empty() && !value.restParameter.empty()) {
        stream << value.restParameter;
    }
    else {
        stream << "(";
        bsl::vector<bsl::string>::const_iterator iter =
            value.positionalParameters.begin();
        const bsl::vector<bsl::string>::const_iterator end =
            value.positionalParameters.end();
        if (iter != end) {
            stream << *iter;
            for (++iter; iter != end; ++iter) {
                stream << " " << *iter;
            }
            if (!value.restParameter.empty()) {
                stream << " . " << value.restParameter;
            }
        }
        stream << ")";
    }

    stream << " ";
    procedureStack.push_back(&value);
    printList(*value.body, k_NO_PARENTHESES);
    procedureStack.pop_back();
    stream << "]";
}

void PrintVisitor::printSet(const Set* set) {
    stream << "#{";

    // in-order depth-first traversal
    class SetVisitor {
        PrintVisitor& print;
        bool          first;

      public:
        explicit SetVisitor(PrintVisitor& print)
        : print(print)
        , first(true) {
        }

        void operator()(const Set* set) {
            if (!set) {
                return;
            }

            // predecessors
            (*this)(set->left());

            // separating space, unless we're first
            if (first) {
                first = false;
            }
            else {
                print.stream << " ";
            }

            // our value
            set->value().apply(print);

            // successors
            (*this)(set->right());
        }
    };

    SetVisitor visit(*this);
    visit(set);

    stream << "}";
}

}  // namespace

void PrintUtil::print(bsl::ostream&      stream,
                      const bdld::Datum& datum,
                      int                typeOffset) {
    PrintVisitor visitor(stream, typeOffset);
    datum.apply(visitor);
}

void PrintUtil::print(bsl::ostream& stream, const Pair& pair, int typeOffset) {
    PrintVisitor visitor(stream, typeOffset);
    visitor.printList(pair);
}

}  // namespace lspcore
