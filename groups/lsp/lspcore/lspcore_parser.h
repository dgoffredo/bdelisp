#ifndef INCLUDED_LSPCORE_PARSER
#define INCLUDED_LSPCORE_PARSER

#include <bdlb_nullablevalue.h>
#include <bdlb_variant.h>
#include <bdld_datum.h>
#include <bsl_string.h>
#include <lspcore_lexer.h>

namespace BloombergLP {
namespace bslma {
class Allocator;
}  // namespace bslma
}  // namespace BloombergLP

namespace lspcore {
namespace bdlb  = BloombergLP::bdlb;
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

struct ParserError {
    bsl::string what;
    LexerToken  where;

    ParserError(bsl::string_view what, const LexerToken& where);
};

class Parser {
    Lexer&            d_lexer;
    int               d_typeOffset;
    LexerToken        d_previousToken;
    bslma::Allocator* d_datumAllocator_p;

  public:
    Parser(Lexer& lexer, int typeOffset, bslma::Allocator* datumAllocator);

    // TODO: document
    bdlb::Variant2<bdld::Datum, ParserError> parse();

  private:
    bdld::Datum parseDatum();
    bdld::Datum parseBoolean(const LexerToken&);
    bdld::Datum parseString(const LexerToken&);
    bdld::Datum parseBytes(const LexerToken&);
    bdld::Datum parseDouble(const LexerToken&);
    bdld::Datum parseDecimal64(const LexerToken&);
    bdld::Datum parseInt32(const LexerToken&);
    bdld::Datum parseInt64(const LexerToken&);
    bdld::Datum parseSymbol(const LexerToken&);
    bdld::Datum parseList(const LexerToken&);
    bdld::Datum parseArray(const LexerToken&);
    bdld::Datum parseMap(const LexerToken&);
    bdld::Datum parseQuoteLike(const LexerToken&);
    bdld::Datum parseComment(const LexerToken&);
    bdld::Datum parseDate(const LexerToken&);
    bdld::Datum parseTime(const LexerToken&);
    bdld::Datum parseDatetime(const LexerToken&);
    bdld::Datum parseDatetimeInterval(const LexerToken&);
    bdld::Datum parseError(const LexerToken&);
    bdld::Datum parseUdt(const LexerToken&);
    bdld::Datum parsePairSecond(const LexerToken&);

    LexerToken next();
};

}  // namespace lspcore

#endif
