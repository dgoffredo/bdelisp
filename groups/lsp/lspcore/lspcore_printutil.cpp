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
#include <bdlt_iso8601util.h>
#include <bdlt_time.h>
#include <bsl_ostream.h>
#include <bsl_string_view.h>
#include <bsls_assert.h>
#include <lspcore_pair.h>
#include <lspcore_printutil.h>
#include <lspcore_symbolutil.h>
#include <lspcore_userdefinedtypes.h>

using namespace BloombergLP;

namespace lspcore {
namespace {

class PrintVisitor {
    bsl::ostream& stream;
    int           typeOffset;

  public:
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
    void printList(const Pair&);
    void printSymbol(const bdld::Datum&);
    template <typename MapRef, typename Entry>
    void printMap(const MapRef&);
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
    baljsn::PrintUtil::printValue(stream, value);
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
            return printSymbol(SymbolUtil::access(value));
        case UserDefinedTypes::e_PROCEDURE:
            BSLS_ASSERT_OPT(!"not implemented");
    }

    // Otherwise, it's some user-defined type we're not aware of. Print its
    // type ID and its address, e.g.: #udt[23 "0x3434324"]
    stream << "#udt[" << value.type() << "\"" << value.data() << "\"]";
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

void PrintVisitor::printList(const Pair& pair) {
    stream << "(";
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

    stream << ")";
}

void PrintVisitor::printSymbol(const bdld::Datum& value) {
    BSLS_ASSERT(value.isString());

    stream << value.theString();
}

}  // namespace

void PrintUtil::print(bsl::ostream&      stream,
                      const bdld::Datum& datum,
                      int                typeOffset) {
    PrintVisitor visitor(stream, typeOffset);
    datum.apply(visitor);
}

}  // namespace lspcore
