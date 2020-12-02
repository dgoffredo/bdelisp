#include <bdld_datum.h>
#include <bsl_cstddef.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>
#include <bsl_string.h>
#include <bsl_string_view.h>
#include <bsl_vector.h>
#include <bslma_default.h>
#include <bsls_assert.h>
#include <lspcore_lexer.h>
#include <lspcore_linecounter.h>
#include <lspcore_listutil.h>
#include <lspcore_symbolutil.h>

namespace {

namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

int lister() {
    lspcore::Lexer    lexer;
    bsl::string subject;
    bsl::ostringstream datumish;
    const int typeOffset = 0;
    bslma::Allocator *allocator = bslma::Default::allocator();

    while (bsl::getline(bsl::cin, subject)) {
        if (const int rc = lexer.reset(subject)) {
            bsl::cerr << "Failed to reset Lexer.\n";
            return rc;
        }

        lspcore::LexerToken token;
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

        bdld::Datum list = lspcore::ListUtil::createList(data, typeOffset, allocator);
        lspcore::ListUtil::hackPrint(list);
        bsl::cout << "\n";

        list = lspcore::ListUtil::createImproperList(data, typeOffset, allocator);
        lspcore::ListUtil::hackPrint(list);
        bsl::cout << "\n";
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
                      << lspcore::SymbolUtil::access(symbol) << "\n";
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
    } else if (which == "lister") {
        return lister();
    }

    BSLS_ASSERT_OPT(which == "counter");
    return counter();
}
