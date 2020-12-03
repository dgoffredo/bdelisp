#include <baljsn_parserutil.h>
#include <bdlb_arrayutil.h>
#include <bdld_datumarraybuilder.h>
#include <bdldfp_decimal.h>
#include <bdlt_date.h>
#include <bdlt_datetime.h>
#include <bdlt_datetimeinterval.h>
#include <bdlt_intervalconversionutil.h>
#include <bdlt_iso8601util.h>
#include <bdlt_time.h>
#include <bsl_sstream.h>
#include <bsls_timeinterval.h>
#include <lspcore_listutil.h>
#include <lspcore_parser.h>
#include <lspcore_symbolutil.h>

using namespace BloombergLP;

namespace lspcore {
namespace {

#define DEFINE_PARSER_ERROR(NAME, MESSAGE)     \
    struct NAME : public ParserError {         \
        explicit NAME(const LexerToken& where) \
        : ParserError(MESSAGE, where) {        \
        }                                      \
    }

DEFINE_PARSER_ERROR(EofError,
                    "encountered the end of input before parsing any value");
DEFINE_PARSER_ERROR(
    BadToken,
    (where.kind == LexerToken::e_INVALID
         ? "unable to parse a token at the beginning of input"
         : "unable to parse a token after the following token"));
DEFINE_PARSER_ERROR(NotAValue, "no value begins with this token");
DEFINE_PARSER_ERROR(InvalidString,
                    "unable to parse string literal as extended JSON");
DEFINE_PARSER_ERROR(InvalidNumber, "unable to parse numeric literal as JSON");
DEFINE_PARSER_ERROR(
    InvalidBase64,
    "unable to parse binary data from base-64 encoded JSON string literal");
DEFINE_PARSER_ERROR(InvalidTemporal,
                    "unable to parse date/time/period from literal");
DEFINE_PARSER_ERROR(IncompleteComment,
                    "encountered a datum commen (i.e. #;...) without a "
                    "complete datum (the \"...\" part)");
DEFINE_PARSER_ERROR(
    IncompleteArray,
    "input ended before the array that starts here was closed");
DEFINE_PARSER_ERROR(
    IncompleteList,
    "input ended before the list/pair that starts here was closed");
DEFINE_PARSER_ERROR(
    IncompletePair,
    "input ended before the pair whose middle is here was closed");
DEFINE_PARSER_ERROR(PairSuffix,
                    "extra data found after second element of pair");
DEFINE_PARSER_ERROR(ErrorIncomplete,
                    "unterminated #error: must be followed by an array");
DEFINE_PARSER_ERROR(ErrorNotArray, "#error must be followed by an array");
DEFINE_PARSER_ERROR(ErrorWrongLength,
                    "#error array must have one or two elements");
DEFINE_PARSER_ERROR(ErrorCodeMustBeInteger,
                    "#error code (first element) must be a 32-bit integer");
DEFINE_PARSER_ERROR(ErrorMessageMustBeString,
                    "#error message (second element) must be a string");
DEFINE_PARSER_ERROR(
    UdtIncomplete,
    "unterminated user-defined type (#udt): must be followed by an array");
DEFINE_PARSER_ERROR(
    UdtNotArray,
    "user-defined type literal (#udt) must be followed by an array");
DEFINE_PARSER_ERROR(
    UdtWrongLength,
    "user-defined type literal (#udt) must have exactly two elements");
DEFINE_PARSER_ERROR(UdtTypeMustBeInteger,
                    "user-defined type literal (#udt) first element, the type "
                    "code, must be a 32-bit integer");
DEFINE_PARSER_ERROR(
    UdtTypeCollidesWithReservedRange,
    "user-defined type literal type code falls in a range that is reserved "
    "for use by this library. The interpreter's configured type offset may be "
    "adjusted to accomodate this user-defined type.");
DEFINE_PARSER_ERROR(UnterminatedQuoteLike,
                    "Quote-like token must be followed by a datum.");

#undef DEFINE_PARSER_ERROR

bool shouldIgnore(const LexerToken& token) {
    switch (token.kind) {
        case LexerToken::e_COMMENT_LINE:
        case LexerToken::e_COMMENT_SHEBANG:
        case LexerToken::e_WHITESPACE:
            return true;
        default:
            return false;
    }
}

bsl::string_view quoteLikeName(LexerToken::Kind kind) {
    switch (kind) {
        case LexerToken::e_QUOTE:
            return "quote";
        case LexerToken::e_QUASIQUOTE:
            return "quasiquote";
        case LexerToken::e_UNQUOTE:
            return "unquote";
        case LexerToken::e_UNQUOTE_SPLICING:
            return "unquote-splicing";
        case LexerToken::e_SYNTAX:
            return "syntax";
        case LexerToken::e_QUASISYNTAX:
            return "quasisyntax";
        case LexerToken::e_UNSYNTAX:
            return "unsyntax";
        default:
            BSLS_ASSERT_OPT(kind == LexerToken::e_UNSYNTAX_SPLICING);
            return "unsyntax-splicing";
    }
}

}  // namespace

ParserError::ParserError(bsl::string_view what, const LexerToken& where)
: what(what)
, where(where) {
}

Parser::Parser(Lexer& lexer, int typeOffset, bslma::Allocator* datumAllocator)
: d_lexer(lexer)
, d_typeOffset(typeOffset)
, d_datumAllocator_p(datumAllocator) {
}

bdlb::Variant2<bdld::Datum, ParserError> Parser::parse() {
    typedef bdlb::Variant2<bdld::Datum, ParserError> Variant;
    try {
        return Variant(parseDatum());
    }
    catch (const ParserError& error) {
        return Variant(error);
    }
}

bdld::Datum Parser::parseDatum() {
    const LexerToken token = next();

    switch (token.kind) {
        case LexerToken::e_EOF:
            throw EofError(token);
        case LexerToken::e_TRUE:
        case LexerToken::e_FALSE:
            return parseBoolean(token);
        case LexerToken::e_STRING:
            return parseString(token);
        case LexerToken::e_BYTES:
            return parseBytes(token);
        case LexerToken::e_DOUBLE:
            return parseDouble(token);
        case LexerToken::e_DECIMAL64:
            return parseDecimal64(token);
        case LexerToken::e_INT32:
            return parseInt32(token);
        case LexerToken::e_INT64:
            return parseInt64(token);
        case LexerToken::e_SYMBOL:
            return parseSymbol(token);
        case LexerToken::e_OPEN_PARENTHESIS:
            return parseList(token);
        case LexerToken::e_OPEN_SQUARE_BRACKET:
            return parseArray(token);
        case LexerToken::e_OPEN_CURLY_BRACE:
            return parseMap(token);
        case LexerToken::e_CLOSE_PARENTHESIS:
        case LexerToken::e_CLOSE_SQUARE_BRACKET:
        case LexerToken::e_CLOSE_CURLY_BRACE:
        case LexerToken::e_PAIR_SEPARATOR:
            throw NotAValue(token);
        case LexerToken::e_QUOTE:
        case LexerToken::e_QUASIQUOTE:
        case LexerToken::e_UNQUOTE:
        case LexerToken::e_UNQUOTE_SPLICING:
        case LexerToken::e_SYNTAX:
        case LexerToken::e_QUASISYNTAX:
        case LexerToken::e_UNSYNTAX:
        case LexerToken::e_UNSYNTAX_SPLICING:
            return parseQuoteLike(token);
        case LexerToken::e_COMMENT_DATUM:
            return parseComment(token);
        case LexerToken::e_DATE:
            return parseDate(token);
        case LexerToken::e_TIME:
            return parseTime(token);
        case LexerToken::e_DATETIME:
            return parseDatetime(token);
        case LexerToken::e_DATETIME_INTERVAL:
            return parseDatetimeInterval(token);
        case LexerToken::e_ERROR_TAG:
            return parseError(token);
        default:
            BSLS_ASSERT_OPT(token.kind == LexerToken::e_USER_DEFINED_TYPE_TAG);
            return parseUdt(token);

            // Impossible cases:
            // - LexerToken::e_COMMENT_LINE (skipped by 'this->next')
            // - LexerToken::e_COMMENT_SHEBANG (skipped by 'this->next')
            // - LexerToken::e_INVALID (never produced by 'Lexer')
            // - LexerToken::e_WHITESPACE (skipped by 'this->next')
    }
}

bdld::Datum Parser::parseBoolean(const LexerToken& token) {
    return bdld::Datum::createBoolean(token.kind == LexerToken::e_TRUE);
}

bdld::Datum Parser::parseString(const LexerToken& token) {
    // Note: The documentation for 'baljsn' says that string decoding agrees
    // with that described in <json.org>. It does not. JSON strings shall not
    // include unescaped control characters, but
    // 'baljsn::ParserUtil::getString' accepts them. This is a reasonable
    // extension, but is contrary to the documentation.
    //
    // The reason I'm making a fuss is that technically BDE could change the
    // function's behavior at any time to adhere to the documentation, and this
    // function ('lspcore::Parser::parseString') would break, because we _do_
    // want to allow for unescaped control characters.
    //
    // Realistically, it is unlikely that BDE will make such a change. I could
    // copy the current implementation of 'baljsn::ParserUtil::getString' to
    // preserve its behavior, but I think that will be unnecessary.
    bsl::string parsed;
    if (baljsn::ParserUtil::getValue(&parsed, token.text)) {
        throw InvalidString(token);
    }

    return bdld::Datum::copyString(parsed, d_datumAllocator_p);
}

bdld::Datum Parser::parseBytes(const LexerToken& token) {
    // Raw bytes are encoded as a base-64 string literal with a prefix, e.g.
    //
    //     #base64"9kDfid899==="
    //
    // If we skip past the leading "#base64", then the JSON decoder can treat
    // it as a base-64 encoded string.
    bsl::string_view base64 = token.text;
    base64.remove_prefix(sizeof "#base64" - 1);
    bsl::vector<char> buffer;
    if (baljsn::ParserUtil::getValue(&buffer, base64)) {
        throw InvalidBase64(token);
    }

    return bdld::Datum::copyBinary(
        buffer.data(), buffer.size(), d_datumAllocator_p);
}

bdld::Datum Parser::parseDouble(const LexerToken& token) {
    // 'remove_suffix(1)' to get rid of the trailing "B"
    bsl::string_view text = token.text;
    text.remove_suffix(1);
    double parsed;
    if (baljsn::ParserUtil::getValue(&parsed, text)) {
        throw InvalidNumber(token);
    }

    return bdld::Datum::createDouble(parsed);
}

bdld::Datum Parser::parseDecimal64(const LexerToken& token) {
    // Decimal64 are the naked decimals, e.g. 34.54 (no special prefix or
    // suffix).
    bdldfp::Decimal64 parsed;
    if (baljsn::ParserUtil::getValue(&parsed, token.text)) {
        throw InvalidNumber(token);
    }

    return bdld::Datum::createDecimal64(parsed, d_datumAllocator_p);
}

bdld::Datum Parser::parseInt32(const LexerToken& token) {
    // int32 are the naked integers, e.g. 1234 (no special suffix).
    int parsed;
    if (baljsn::ParserUtil::getValue(&parsed, token.text)) {
        throw InvalidNumber(token);
    }

    return bdld::Datum::createInteger(parsed);
}

bdld::Datum Parser::parseInt64(const LexerToken& token) {
    // int64 literals require an "L" suffix. Strip away that suffix before
    // parsing.
    bsl::string_view text = token.text;
    text.remove_suffix(1);
    bsls::Types::Int64 parsed;
    if (baljsn::ParserUtil::getValue(&parsed, text)) {
        throw InvalidNumber(token);
    }

    return bdld::Datum::createInteger64(parsed, d_datumAllocator_p);
}

bdld::Datum Parser::parseSymbol(const LexerToken& token) {
    // symbols are verbatim
    return SymbolUtil::create(token.text, d_typeOffset, d_datumAllocator_p);
}

bdld::Datum Parser::parseList(const LexerToken& token) {
    bsl::vector<bdld::Datum> elements;

    // Consume zero or more elements, and if we hit a "." make sure that
    // there's exactly one element remaining afterward. We're done when we
    // find a ")".
    for (;;) {
        try {
            elements.push_back(parseDatum());
        }
        catch (const NotAValue& error) {
            switch (error.where.kind) {
                case LexerToken::e_CLOSE_PARENTHESIS:  // all done
                    return ListUtil::createList(
                        elements, d_typeOffset, d_datumAllocator_p);
                case LexerToken::e_PAIR_SEPARATOR:  // one more
                    elements.push_back(parsePairSecond(error.where));
                    return ListUtil::createImproperList(
                        elements, d_typeOffset, d_datumAllocator_p);
                default:  // unexpected punctuation (e.g. "}")
                    throw;
            }
        }
        catch (const EofError&) {
            throw IncompleteList(token);
        }
    }
}

bdld::Datum Parser::parsePairSecond(const LexerToken& token) {
    // Parse the foo in (anything before the dot . foo)
    // The specified 'token' is the "."
    // There must be exactly one datum following the ".", and then the closing
    // parenthesis.
    bdld::Datum last;
    try {
        last = parseDatum();
    }
    catch (const NotAValue& error) {
        if (error.where.kind == LexerToken::e_CLOSE_PARENTHESIS) {
            throw IncompletePair(token);
        }
        throw;  // some other unexpected punctutation
    }
    catch (const EofError&) {
        throw IncompletePair(token);
    }

    const LexerToken extra = next();
    if (extra.kind != LexerToken::e_CLOSE_PARENTHESIS) {
        throw PairSuffix(extra);
    }

    return last;
}

bdld::Datum Parser::parseArray(const LexerToken& token) {
    bdld::DatumArrayBuilder builder(d_datumAllocator_p);

    // Consume array elements (datums) until either we hit "]" (done), we hit
    // EOF (error), or some other exception is thrown (implicit error).
    for (;;) {
        try {
            builder.pushBack(parseDatum());
        }
        catch (const NotAValue& error) {
            if (error.where.kind == LexerToken::e_CLOSE_SQUARE_BRACKET) {
                // finished
                return builder.commit();
            }
            // some other unexpected punctutation: propagate the error
            throw;
        }
        catch (const EofError&) {
            throw IncompleteArray(token);
        }
    }
}

bdld::Datum Parser::parseMap(const LexerToken&) {
    return bdld::Datum::createNull(); /*TODO*/
}

bdld::Datum Parser::parseQuoteLike(const LexerToken& token) {
    // A quote-like form is one preceded by special punctuation, like a single
    // quote (hence the name "quote-like"). Here are some examples where the
    // list "(foo bar)" is made quote-like by adding a preceding token:
    //
    //     '(foo bar)
    //     `(foo bar)
    //     ,(foo bar)
    //     ,@(foo bar)
    //     #'(foo bar)
    //     #`(foo bar)
    //     #,(foo bar)
    //
    // It's not just lists, either, it can be any datum.
    //
    // Each of these quote-like syntaxes is a shorthand for a corresponding
    // special form, e.g.
    //
    //     '(foo bar)
    //
    // means
    //
    //     (quote (foo bar))
    //
    // The parser thus aways produces a two-element list whose first element is
    // a hard-coded symbol and whose second element is the datum following the
    // quote-like token.

    bdld::Datum  data[2];
    bdld::Datum& symbol   = data[0];
    bdld::Datum& argument = data[1];

    try {
        argument = parseDatum();
    }
    catch (const EofError&) {
        throw UnterminatedQuoteLike(token);
    }
    catch (const NotAValue&) {
        throw UnterminatedQuoteLike(token);
    }

    symbol = SymbolUtil::create(
        quoteLikeName(token.kind), d_typeOffset, d_datumAllocator_p);

    return ListUtil::createList(
        data, bdlb::ArrayUtil::end(data), d_typeOffset, d_datumAllocator_p);
}

bdld::Datum Parser::parseComment(const LexerToken& token) {
    // Here are what datum comments look like:
    //
    //     #;some-datum-goes-here
    //     #;(could be "like this")
    //     #;{"or" 'any "single" 'value}
    //
    // e.g. in context:
    //
    //     (this is a #;list with some #;["parts commented" out])
    //
    // The token passed to us is the "#;". We need to parse one additional
    // datum. If we can't, then that's an error.
    //
    // If we can parse one additional datum, then we discard what we parsed,
    // and return the result of attempting to parse yet another datum,
    // effectively skipping the datum that immediately followed the "#;" token.
    try {
        (void)parseDatum();
    }
    catch (const EofError&) {
        throw IncompleteComment(token);
    }

    return parseDatum();
}

bdld::Datum Parser::parseDate(const LexerToken& token) {
    bdlt::Date parsed;
    if (bdlt::Iso8601Util::parse(
            &parsed, token.text.data(), token.text.size())) {
        throw InvalidTemporal(token);
    }

    return bdld::Datum::createDate(parsed);
}

bdld::Datum Parser::parseTime(const LexerToken& token) {
    bdlt::Time parsed;
    if (bdlt::Iso8601Util::parse(
            &parsed, token.text.data(), token.text.size())) {
        throw InvalidTemporal(token);
    }

    return bdld::Datum::createTime(parsed);
}

bdld::Datum Parser::parseDatetime(const LexerToken& token) {
    bdlt::Datetime parsed;
    if (bdlt::Iso8601Util::parse(
            &parsed, token.text.data(), token.text.size())) {
        throw InvalidTemporal(token);
    }

    return bdld::Datum::createDatetime(parsed, d_datumAllocator_p);
}

bdld::Datum Parser::parseDatetimeInterval(const LexerToken& token) {
    // A time period, e.g. #P10W (ten weeks) starts with a "#" character in
    // this lisp. The rest conforms to ISO-8601, so omit the first character
    // before parsing.
    bsl::string_view text = token.text;
    text.remove_prefix(1);
    bsls::TimeInterval parsed;
    if (bdlt::Iso8601Util::parse(&parsed, text.data(), text.size())) {
        throw InvalidTemporal(token);
    }

    return bdld::Datum::createDatetimeInterval(
        bdlt::IntervalConversionUtil::convertToDatetimeInterval(parsed),
        d_datumAllocator_p);
}

bdld::Datum Parser::parseError(const LexerToken& token) {
    // An error is represented as an error tag ("#error") followed by an
    // array containing either one or two elements. The first element is
    // always the integer code of the error. The optional second element
    // is a string literal error message. If the error message is omitted,
    // then it is considered empty in the resulting error datum, e.g.
    //
    //     #error[10]
    //     #error[10 "this is serious, guys!"]

    bdld::Datum datum;
    try {
        datum = parseDatum();
    }
    catch (const EofError&) {
        throw ErrorIncomplete(token);
    }

    if (!datum.isArray()) {
        throw ErrorNotArray(token);
    }

    const bdld::DatumArrayRef array = datum.theArray();
    if (array.length() < 1 || array.length() > 2) {
        throw ErrorWrongLength(token);
    }

    const bdld::Datum code = array[0];
    if (!code.isInteger()) {
        throw ErrorCodeMustBeInteger(token);
    }

    if (array.length() == 1) {
        // There's no message, so return the error with just a code.
        return bdld::Datum::createError(code.theInteger());
    }

    const bdld::Datum message = array[1];
    if (!message.isString()) {
        throw ErrorMessageMustBeString(token);
    }

    return bdld::Datum::createError(
        code.theInteger(), message.theString(), d_datumAllocator_p);
}

bdld::Datum Parser::parseUdt(const LexerToken& token) {
    // A user-defined type literal is represented as a udt tag ("#udt")
    // followed by an array containing two elements. The first element is the
    // integer "type" of the user-defined type (that which distinguishes it
    // from other types of user-defined types). The second element is
    // implementation-defined. When printing, if no lisp serialization of the
    // type is known, then a string containing the "0x" prefixed hexadecimal
    // address of the object in memory, e.g.
    //
    //     #udt[42 "0xDEADBEEF"]
    //
    // indicates an instance of user-defined type of type 42 and at address
    // 0xDEADBEEF.
    //
    // There's currently no mechanism by which user-defined type parsers can be
    // registered with this library, so for now these types are parsed with the
    // indicated "type" but with a "data" pointer of zero, i.e. the second
    // element of the array is ignored. Note that this means that the parse is
    // lossy, e.g.
    //
    //     #udt[42 "0xDEADBEEF"]
    //
    // parses as if it were
    //
    //     #udt[42 "0x0"]
    //
    // Some user-defined type codes are reserved by this library, subject to a
    // configurable offset (e.g. 'd_typeOffset'). Attempts to parse
    // user-defined types with codes conflicting with those used by this
    // library will result in an error.

    bdld::Datum datum;
    try {
        datum = parseDatum();
    }
    catch (const EofError&) {
        throw UdtIncomplete(token);
    }

    if (!datum.isArray()) {
        throw UdtNotArray(token);
    }

    const bdld::DatumArrayRef array = datum.theArray();
    if (array.length() != 2) {
        throw UdtWrongLength(token);
    }

    const bdld::Datum type = array[0];
    if (!type.isInteger()) {
        throw UdtTypeMustBeInteger(token);
    }
    // Note that the second element of the array is (currently) ignored.

    const int code = type.theInteger();
    if (code >= d_typeOffset &&
        code < int(d_typeOffset + UserDefinedTypes::k_COUNT)) {
        throw UdtTypeCollidesWithReservedRange(token);
    }

    return bdld::Datum::createUdt(0, code);
}

LexerToken Parser::next() {
    LexerToken token;
    do {
        if (d_lexer.next(&token)) {
            throw BadToken(d_previousToken);
        }
        BSLS_ASSERT(token.kind != LexerToken::e_INVALID);
        d_previousToken = token;
    } while (shouldIgnore(token));

    return token;
}

}  // namespace lspcore
