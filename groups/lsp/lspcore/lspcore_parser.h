#ifndef INCLUDED_LSPCORE_PARSER
#define INCLUDED_LSPCORE_PARSER

#include <bdlb_nullablevalue.h>
#include <bdlb_variant.h>
#include <bdld_datum.h>
#include <bsl_string.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
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

// Note:
// 
// This parser generates garbage. Some of it could be avoided, but not
// all of it. The fundamental problem is that 'bdld::Datum::destroy' does not
// know how to destroy objects referred to by user-defined types, and there is
// no way to write an external "destroy" function without access to the
// internals of 'bdld::Datum'.
//
// One way to deal with the garbage is to use two allocators: one for the
// resulting 'bdld::Datum', and one for use by the parser. When the parser
// returns a result, clone it using the allocator for the 'bdld::Datum', and
// then '.release()' the parser's allocator. The 'Parser' class does not
// provide this facility, because I don't intend to need it.

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

    bool appendMapItem(
        bsl::vector<bsl::pair<bdld::Datum, bdld::Datum> >* items,
        LexerToken                                         openCurly);
};

}  // namespace lspcore

#endif
