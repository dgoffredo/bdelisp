#include <bdlb_arrayutil.h>
#include <bdlb_variant.h>
#include <bdld_datum.h>
#include <bsl_cstddef.h>
#include <bsl_iomanip.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bsls_assert.h>
#include <lspcore_arithmeticutil.h>
#include <lspcore_builtinprocedures.h>
#include <lspcore_interpreter.h>
#include <lspcore_lexer.h>
#include <lspcore_linecounter.h>
#include <lspcore_listutil.h>
#include <lspcore_parser.h>
#include <lspcore_printutil.h>
#include <lspcore_set.h>
#include <lspcore_symbolutil.h>

namespace {

namespace bdlb  = BloombergLP::bdlb;
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

bool lessThan(const bdld::Datum& left, const bdld::Datum& right) {
    BSLS_ASSERT_OPT(left.isInteger());
    BSLS_ASSERT_OPT(right.isInteger());

    return left.theInteger() < right.theInteger();
}

int sets() {
    const int           typeOffset = 0;
    bslma::Allocator*   allocator  = bslma::Default::allocator();
    const lspcore::Set* set        = 0;

    lspcore::PrintUtil::print(bsl::cout,
                              lspcore::Set::toList(set, typeOffset, allocator),
                              typeOffset);
    bsl::cout << "\n";

    // const int values[] = { 10, 43, 134, -8, 0, 2, 2, 2 };
    const int values[] = { 10, 43, 134, -8, 0,  2,  2,  2,  1,  2,  3,  4,
                           5,  6,  7,   8,  9,  10, 11, 12, 13, 14, 15, 16,
                           17, 18, 19,  20, 21, 22, 23, 24, 25, 26, 27, 28,
                           29, 30, 31,  32, 33, 34, 35, 36, 37, 38, 39, 40,
                           41, 42, 43,  44, 45, 46, 47, 48, 49, 50, 51, 52,
                           53, 54, 55,  56, 57, 58, 59, 60, 61, 62, 63, 64,
                           65, 66, 67,  68, 69, 70, 71, 72, 73, 74, 75, 76,
                           77, 78, 79,  80, 81, 82, 83, 84, 85, 86, 87, 88,
                           89, 90, 91,  92, 93, 94, 95, 96, 97, 98, 99, 100 };
    for (const int* iter = values; iter != bdlb::ArrayUtil::end(values);
         ++iter) {
        set = lspcore::Set::insert(
            set, bdld::Datum::createInteger(*iter), &lessThan, allocator);
        bsl::cout << "after adding " << *iter << ":    ";
        lspcore::PrintUtil::print(
            bsl::cout,
            lspcore::Set::toList(set, typeOffset, allocator),
            typeOffset);
        bsl::cout << "\n";
    }

    const int checks[] = { 10, -1, -8, 43, 44 };
    for (const int* iter = checks; iter != bdlb::ArrayUtil::end(checks);
         ++iter) {
        bsl::cout << "Is " << *iter << " in the set?    " << bsl::boolalpha
                  << lspcore::Set::contains(
                         set, bdld::Datum::createInteger(*iter), &lessThan)
                  << "\n";
    }

    for (const int* iter = values; iter != bdlb::ArrayUtil::end(values);
         ++iter) {
        bsl::cout << "Removing " << *iter << "\n";

        set = lspcore::Set::remove(
            set, bdld::Datum::createInteger(*iter), &lessThan, allocator);

        bsl::cout << "after removing " << *iter << ":    ";
        lspcore::PrintUtil::print(
            bsl::cout,
            lspcore::Set::toList(set, typeOffset, allocator),
            typeOffset);
        bsl::cout << "\n";

        bsl::cout << "Is " << *iter << " in the set?    " << bsl::boolalpha
                  << lspcore::Set::contains(
                         set, bdld::Datum::createInteger(*iter), &lessThan)
                  << "\n";
    }

    return 0;
}

int interpreter() {
    lspcore::Lexer       lexer;
    bsl::string          subject;
    const int            typeOffset = 0;
    bslma::Allocator*    allocator  = bslma::Default::allocator();
    lspcore::Parser      parser(lexer, typeOffset, allocator);
    lspcore::Interpreter interpreter(typeOffset, allocator);

    int rc;
    struct Entry {
        const char*                              name;
        lspcore::NativeProcedureUtil::Signature* function;
    } const procedures[] = {
        { "+", &lspcore::ArithmeticUtil::add },
        { "-", &lspcore::ArithmeticUtil::subtract },
        { "*", &lspcore::ArithmeticUtil::multiply },
        { "/", &lspcore::ArithmeticUtil::divide },
        { "=", &lspcore::ArithmeticUtil::equal },
        { "pair?", &lspcore::BuiltinProcedures::isPair },
        { "pair", &lspcore::BuiltinProcedures::pair },
        { "pair-first", &lspcore::BuiltinProcedures::pairFirst },
        { "pair-second", &lspcore::BuiltinProcedures::pairSecond },
        { "null?", &lspcore::BuiltinProcedures::isNull },
        { "equal?", &lspcore::BuiltinProcedures::equal },
        { "list", &lspcore::BuiltinProcedures::list },
        { "apply", &lspcore::BuiltinProcedures::apply },
        { "raise", &lspcore::BuiltinProcedures::raise },
        { "set", &lspcore::BuiltinProcedures::set },
        { "set-contains?", &lspcore::BuiltinProcedures::setContains },
        { "set-insert", &lspcore::BuiltinProcedures::setInsert },
        { "set-remove", &lspcore::BuiltinProcedures::setRemove }
    };

    for (const Entry* entry = procedures;
         entry != bdlb::ArrayUtil::end(procedures);
         ++entry) {
        const int rc =
            interpreter.defineNativeProcedure(entry->name, entry->function);
        BSLS_ASSERT_OPT(rc == 0);
    }

    bdlb::Variant2<bdld::Datum, lspcore::ParserError> parserResult;
    // while (bsl::cout << "\nbdelisp> ", bsl::getline(bsl::cin, subject)) {
    while (bsl::cout << "\nbdelisp> ", bsl::getline(bsl::cin, subject, '$')) {
        int rc = lexer.reset(subject);
        BSLS_ASSERT_OPT(rc == 0);

        parserResult = parser.parse();
        if (parserResult.is<bdld::Datum>()) {
            const bdld::Datum& expression = parserResult.the<bdld::Datum>();
            const bdld::Datum  result     = interpreter.evaluate(expression);
            lspcore::PrintUtil::print(bsl::cout, result, typeOffset);
        }
        else {
            const lspcore::ParserError& error =
                parserResult.the<lspcore::ParserError>();
            bsl::cout << "Parser error: " << error.what
                      << "\ntoken: " << error.where;
        }
    }

    return 0;
}

int regurgitate() {
    lspcore::Lexer    lexer;
    bsl::string       subject;
    const int         typeOffset = 0;
    bslma::Allocator* allocator  = bslma::Default::allocator();
    lspcore::Parser   parser(lexer, typeOffset, allocator);
    bdlb::Variant2<bdld::Datum, lspcore::ParserError> result;

    bsl::ostringstream input;
    input << bsl::cin.rdbuf();
    subject = input.str();

    const int rc = lexer.reset(subject);
    BSLS_ASSERT_OPT(rc == 0);

    result = parser.parse();
    if (result.is<bdld::Datum>()) {
        const bdld::Datum& datum = result.the<bdld::Datum>();
        bsl::cout << datum << "\n";
        lspcore::PrintUtil::print(bsl::cout, datum, typeOffset);
    }
    else {
        const lspcore::ParserError& error = result.the<lspcore::ParserError>();
        bsl::cout << "Error: " << error.what << "\ntoken: " << error.where;
    }
    bsl::cout << "\n\n";

    return 0;
}

int regurgitator() {
    lspcore::Lexer    lexer;
    bsl::string       subject;
    const int         typeOffset = 0;
    bslma::Allocator* allocator  = bslma::Default::allocator();
    lspcore::Parser   parser(lexer, typeOffset, allocator);

    bdlb::Variant2<bdld::Datum, lspcore::ParserError> result;
    while (bsl::getline(bsl::cin, subject)) {
        int rc = lexer.reset(subject);
        BSLS_ASSERT_OPT(rc == 0);

        result = parser.parse();
        if (result.is<bdld::Datum>()) {
            const bdld::Datum& datum = result.the<bdld::Datum>();
            bsl::cout << datum << "\n";
            lspcore::PrintUtil::print(bsl::cout, datum, typeOffset);
        }
        else {
            const lspcore::ParserError& error =
                result.the<lspcore::ParserError>();
            bsl::cout << "Error: " << error.what << "\ntoken: " << error.where;
        }
        bsl::cout << "\n\n";
    }

    return 0;
}

int lister() {
    lspcore::Lexer     lexer;
    bsl::string        subject;
    bsl::ostringstream datumish;
    const int          typeOffset = 0;
    bslma::Allocator*  allocator  = bslma::Default::allocator();

    while (bsl::getline(bsl::cin, subject)) {
        if (const int rc = lexer.reset(subject)) {
            bsl::cerr << "Failed to reset Lexer.\n";
            return rc;
        }

        lspcore::LexerToken      token;
        bsl::vector<bdld::Datum> data;
        while (lexer.next(&token) == 0 &&
               token.kind != lspcore::LexerToken::e_EOF) {
            if (token.kind == lspcore::LexerToken::e_WHITESPACE) {
                continue;
            }
            datumish << token;
            data.push_back(bdld::Datum::copyString(datumish.str(), allocator));
            datumish.str(bsl::string());
        }

        bdld::Datum list =
            lspcore::ListUtil::createImproperList(data, typeOffset, allocator);
        lspcore::PrintUtil::print(bsl::cout, list, typeOffset);
        bsl::cout << "\n\n";

        list = lspcore::ListUtil::createList(data, typeOffset, allocator);
        lspcore::PrintUtil::print(bsl::cout, list, typeOffset);
        bsl::cout << "\n\n";
    }

    std::cout << "\n";
    return 0;
}

int lexer() {
    lspcore::Lexer    lexer;
    bsl::stringstream input;

    input << bsl::cin.rdbuf();
    bsl::string subject = input.str();

    if (const int rc = lexer.reset(subject)) {
        bsl::cerr << "Failed to reset Lexer.\n";
        return rc;
    }

    lspcore::LexerToken token;
    while (lexer.next(&token) == 0 &&
           token.kind != lspcore::LexerToken::e_EOF) {
        if (token.kind == lspcore::LexerToken::e_SYMBOL) {
            bsl::cout << token << "\n";
            bdld::Datum symbol = lspcore::SymbolUtil::create(
                token.text, 0, bslma::Default::allocator());
            bsl::cout << "The symbol UDT: " << symbol << "\n"
                      << "The symbol string: "
                      << lspcore::SymbolUtil::name(symbol) << "\n";
        }
        else if (token.kind != lspcore::LexerToken::e_WHITESPACE) {
            bsl::cout << token << "\n";
        }
    }

    std::cout << "\n";

    return 0;
}

int counter() {
    const bsl::string_view string = "I'm a giant\nfish and now\nso can you!";
    lspcore::LineCounter   counter;
    counter.reset(string.data());

    const auto lookFor = [&](bsl::string_view what) {
        std::cout << "string: " << string << "\n";
        std::cout << "Looking for: " << what << "\n";

        bsl::size_t offset = string.find(what);
        BSLS_ASSERT(offset != bsl::string_view::npos);
        counter.advanceToOffset(string.data(), offset);

        std::cout << "counter is at line " << counter.line() << " column "
                  << counter.column() << " corresponding to offset "
                  << counter.offset() << "\n\n";
    };

    lookFor("\n");

    bsl::string what;
    while (bsl::getline(std::cin, what)) {
        lookFor(what);
    }

    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    BSLS_ASSERT_OPT(argc == 2);

    bsl::string_view which = argv[1];
    if (which == "lexer") {
        return lexer();
    }
    else if (which == "lister") {
        return lister();
    }
    else if (which == "regurgitator") {
        return regurgitator();
    }
    else if (which == "regurgitate") {
        return regurgitate();
    }
    else if (which == "interpreter") {
        return interpreter();
    }
    else if (which == "sets") {
        return sets();
    }

    BSLS_ASSERT_OPT(which == "counter");
    return counter();
}
