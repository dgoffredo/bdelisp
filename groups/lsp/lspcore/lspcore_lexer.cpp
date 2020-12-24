#include <baljsn_printutil.h>
#include <bdlb_arrayutil.h>
#include <bsl_ostream.h>
#include <bsls_assert.h>
#include <lspcore_lexer.h>

using namespace BloombergLP;

namespace lspcore {
namespace {

// In order to distinguish individual tokens from their concatenations, we need
// to surround certain patterns with lookbehinds and lookaheads. For example,
// is "2020-11-29" a date, or is is three integers? It wouldn't be enough to
// require that the tokens be surrounded by whitespace, because the "42" in
// "(42)" or in "'('42[x])" is still a valid integer token. What is needed is
// to distinguish a pattern as "delimited" by constraining what may precede and
// follow it.
#define DELIMITED_LEFT(PATTERN)                                            \
    /* allowed preceding character (lookbehind) */                         \
    /* (either beginning of string, or an allowed character) */            \
    R"re((?:(?<=^)|(?<=[\s[\](){}",'`@!;])))re" /* the original pattern */ \
        PATTERN

#define DELIMITED_RIGHT(PATTERN)                          \
    PATTERN                                               \
    /* allowed following character (lookahead) */         \
    /* (either end of string, or an allowed character) */ \
    R"re((?:(?=$)|(?=[\s[\](){}";])))re"

#define DELIMITED(PATTERN) DELIMITED_RIGHT(DELIMITED_LEFT(PATTERN))

// The regular expression pattern used to match tokens is an alternation among
// various subpatterns, one for each type of token. Each subpattern must _not_
// capture any subgroups.
//
// 'k_subpatterns' maps subpattern index to token type (kind), except that it's
// offset by one so that the first subpattern is at index zero, instead of the
// entire match being at index zero. See its use in 'Lexer::next' for more
// information.
const LexerToken::Kind k_subpatterns[] = {
#define TRUE DELIMITED(R"re(#t(?:rue)?)re")
    LexerToken::e_TRUE,
#define FALSE DELIMITED(R"re(#f(?:alse)?)re")
    LexerToken::e_FALSE,
#define STRING R"re("(?:[^"]|\\.)*")re"
    LexerToken::e_STRING,
// Restricting bytes to valid base64 will happen in the parser.
#define BYTES "#base64" STRING
    LexerToken::e_BYTES,
// Restriction to allowed numeric values will happen in the parser.
// This pattern is based on the railroad diagram for numbers at <json.org>,
// except that the decimal part is required (to distinguish from an int).
#define DECIMAL_RAW R"re(-?(?:0|[1-9]\d*)[,.]\d+(?:[eE][+-]?\d+)?)re"
#define DOUBLE      DELIMITED(DECIMAL_RAW "B")
    LexerToken::e_DOUBLE,
#define DECIMAL64 DELIMITED(DECIMAL_RAW)
    LexerToken::e_DECIMAL64,
#define INT_RAW R"re(-?(?:0|[1-9]\d*))re"
#define INT32   DELIMITED(INT_RAW)
    LexerToken::e_INT32,
#define INT64 DELIMITED(INT_RAW "L")
    LexerToken::e_INT64,
#define WHITESPACE R"re(\s+)re"
    LexerToken::e_WHITESPACE,
#define OPEN_PARENTHESIS R"re(\()re"
    LexerToken::e_OPEN_PARENTHESIS,
#define CLOSE_PARENTHESIS R"re(\))re"
    LexerToken::e_CLOSE_PARENTHESIS,
#define OPEN_SQUARE_BRACKET R"re(\[)re"
    LexerToken::e_OPEN_SQUARE_BRACKET,
#define CLOSE_SQUARE_BRACKET R"re(\])re"
    LexerToken::e_CLOSE_SQUARE_BRACKET,
#define OPEN_CURLY_BRACE R"re(\{)re"
    LexerToken::e_OPEN_CURLY_BRACE,
#define CLOSE_CURLY_BRACE R"re(\})re"
    LexerToken::e_CLOSE_CURLY_BRACE,
#define OPEN_SET_BRACE "#{"
    LexerToken::e_OPEN_SET_BRACE,
#define QUOTE R"re(')re"
    LexerToken::e_QUOTE,
#define QUASIQUOTE R"re(`)re"
    LexerToken::e_QUASIQUOTE,
#define UNQUOTE R"re(,(?!@))re"
    LexerToken::e_UNQUOTE,
#define UNQUOTE_SPLICING R"re(,@)re"
    LexerToken::e_UNQUOTE_SPLICING,
#define SYNTAX R"re(#')re"
    LexerToken::e_SYNTAX,
#define QUASISYNTAX R"re(#`)re"
    LexerToken::e_QUASISYNTAX,
#define UNSYNTAX R"re(#,(?!@))re"
    LexerToken::e_UNSYNTAX,
#define UNSYNTAX_SPLICING R"re(#,@)re"
    LexerToken::e_UNSYNTAX_SPLICING,
#define COMMENT_LINE R"re(;[^\n]*(?:\n|$))re"
    LexerToken::e_COMMENT_LINE,
#define COMMENT_DATUM R"re(#;)re"
    LexerToken::e_COMMENT_DATUM,
#define COMMENT_SHEBANG R"re(#![^\n]*(?:\n|$))re"
    LexerToken::e_COMMENT_SHEBANG,
#define DATE_RAW R"re(\d\d\d\d-\d\d-\d\d)re"
#define DATE     DELIMITED(DATE_RAW)
    LexerToken::e_DATE,
#define TIME_RAW R"re(\d\d:\d\d(?::\d\d(?:[,.]\d+)?)?)re"
#define TIME     DELIMITED(TIME_RAW)
    LexerToken::e_TIME,
#define DATETIME DELIMITED(DATE_RAW "T" TIME_RAW)
    LexerToken::e_DATETIME,
// 'DATETIME_INTERVAL' is too permissive, but the parser is stricter.
#define DATETIME_INTERVAL DELIMITED(R"re(#P[.,WDTHMS0-9]+)re")
    LexerToken::e_DATETIME_INTERVAL,
#define ERROR_TAG DELIMITED(R"re(#error)re")
    LexerToken::e_ERROR_TAG,
#define USER_DEFINED_TYPE_TAG DELIMITED(R"re(#udt)re")
    LexerToken::e_USER_DEFINED_TYPE_TAG,
#define PAIR_SEPARATOR DELIMITED(R"re(\.)re")
    LexerToken::e_PAIR_SEPARATOR
};

#define OR ")|("

// The subpattern macros (e.g. 'STRING', 'UNQUOTE') must appear within
// 'k_PATTERN' in the same order as defined above.
const char k_TOKEN_PATTERN[] =
    "(" TRUE OR FALSE OR STRING OR BYTES OR DOUBLE OR DECIMAL64 OR INT32 OR
        INT64 OR WHITESPACE OR OPEN_PARENTHESIS OR CLOSE_PARENTHESIS OR
            OPEN_SQUARE_BRACKET OR CLOSE_SQUARE_BRACKET OR OPEN_CURLY_BRACE OR
                CLOSE_CURLY_BRACE OR OPEN_SET_BRACE OR QUOTE OR QUASIQUOTE OR
                    UNQUOTE OR UNQUOTE_SPLICING OR SYNTAX OR QUASISYNTAX OR
                        UNSYNTAX OR UNSYNTAX_SPLICING OR COMMENT_LINE OR
                            COMMENT_DATUM OR COMMENT_SHEBANG OR DATE OR TIME OR
                                DATETIME OR DATETIME_INTERVAL OR ERROR_TAG OR
                                    USER_DEFINED_TYPE_TAG OR PAIR_SEPARATOR
    ")";

// Symbols are special. Rather than being one of the alternatives in the
// 'k_TOKEN_PATTERN', the symbol pattern is instead a fallback. The logic is
// "if k_TOKEN_PATTERN" doesn't match a section of the input, and that section
// exactly matches k_SYMBOL_PATTERN, then that section is a token."
//
// This allows the symbol pattern to be permissive without having to clutter
// the token subpatterns for even more exceptions.
const char k_SYMBOL_PATTERN[] =
    DELIMITED_RIGHT(R"re(^[^#\s"()[\]{}'`,][^\s"()[\]{}'`,]*)re");

#undef DELIMITED_LEFT
#undef DELIMITED_RIGHT
#undef DELIMITED

#undef TRUE
#undef FALSE
#undef STRING
#undef BYTES
#undef DECIMAL_RAW
#undef DOUBLE
#undef DECIMAL64
#undef INT_RAW
#undef INT32
#undef INT64
#undef WHITESPACE
#undef OPEN_PARENTHESIS
#undef CLOSE_PARENTHESIS
#undef OPEN_SQUARE_BRACKET
#undef CLOSE_SQUARE_BRACKET
#undef OPEN_CURLY_BRACE
#undef CLOSE_CURLY_BRACE
#undef QUOTE
#undef QUASIQUOTE
#undef UNQUOTE
#undef UNQUOTE_SPLICING
#undef SYNTAX
#undef QUASISYNTAX
#undef UNSYNTAX
#undef UNSYNTAX_SPLICING
#undef COMMENT_LINE
#undef COMMENT_DATUM
#undef COMMENT_SHEBANG
#undef DATE_RAW
#undef DATE
#undef TIME_RAW
#undef TIME
#undef DATETIME
#undef DATETIME_INTERVAL
#undef ERROR_TAG
#undef USER_DEFINED_TYPE_TAG
#undef PAIR_SEPARATOR

#undef OR

int prepare(bdlpcre::RegEx* regex, const char* pattern) {
    BSLS_ASSERT(regex);

    const int flags = bdlpcre::RegEx::k_FLAG_UTF8 |
                      bdlpcre::RegEx::k_FLAG_DOTMATCHESALL |
                      bdlpcre::RegEx::k_FLAG_JIT;
    bsl::string error;
    bsl::size_t offset;

    // I'm happy to assume that the pattern is correct, but I wonder if this
    // can fail due to resource exhaustion. That's why I have error handling.
    return regex->prepare(&error, &offset, pattern, flags);
}

// Oh C++03...
typedef bsl::vector<bsl::pair<bsl::size_t, bsl::size_t> >::const_iterator
    MatchIter;

bool validOffset(const bsl::pair<bsl::size_t, bsl::size_t>& match) {
    return match.first != bdlpcre::RegEx::k_INVALID_OFFSET;
}

}  // namespace

const char* LexerToken::toAscii(LexerToken::Kind kind) {
#define CASE(NAME) \
    case e_##NAME: \
        return #NAME;

    switch (kind) {
        CASE(INVALID)
        CASE(EOF)
        CASE(TRUE)
        CASE(FALSE)
        CASE(STRING)
        CASE(BYTES)
        CASE(DOUBLE)
        CASE(DECIMAL64)
        CASE(INT32)
        CASE(INT64)
        CASE(SYMBOL)
        CASE(WHITESPACE)
        CASE(OPEN_PARENTHESIS)
        CASE(CLOSE_PARENTHESIS)
        CASE(OPEN_SQUARE_BRACKET)
        CASE(CLOSE_SQUARE_BRACKET)
        CASE(OPEN_CURLY_BRACE)
        CASE(CLOSE_CURLY_BRACE)
        CASE(QUOTE)
        CASE(QUASIQUOTE)
        CASE(UNQUOTE)
        CASE(UNQUOTE_SPLICING)
        CASE(SYNTAX)
        CASE(QUASISYNTAX)
        CASE(UNSYNTAX)
        CASE(UNSYNTAX_SPLICING)
        CASE(COMMENT_LINE)
        CASE(COMMENT_DATUM)
        CASE(COMMENT_SHEBANG)
        CASE(DATE)
        CASE(TIME)
        CASE(DATETIME)
        CASE(DATETIME_INTERVAL)
        CASE(ERROR_TAG)
        CASE(USER_DEFINED_TYPE_TAG)
        default:
            BSLS_ASSERT(kind == e_PAIR_SEPARATOR);
            return "PAIR_SEPARATOR";
    }

#undef CASE
}

LexerToken::LexerToken()
: kind()
, text()
, offset()
, beginLine()
, beginColumn()
, endLine()
, endColumn() {
}

bsl::ostream& operator<<(bsl::ostream& stream, const LexerToken& token) {
    // e.g. <10:2 COMMENT_SHEBANG ";!">
    // where the text is JSON quoted

    stream << "<" << token.beginLine << ":" << token.beginColumn << " "
           << LexerToken::toAscii(token.kind) << " ";

    // 'printString' returns a nonzero value when the string is not valid
    // UTF-8. That case I ignore.
    baljsn::PrintUtil::printString(stream, token.text);

    return stream << ">";
}

Lexer::Lexer(bslma::Allocator* allocator)
: d_results(allocator)
, d_tokenRegex(allocator)
, d_symbolRegex(allocator) {
}

int Lexer::reset(bsl::string_view subject) {
    if (!d_tokenRegex.isPrepared()) {
        if (prepare(&d_tokenRegex, k_TOKEN_PATTERN)) {
            return 1;
        }
    }

    if (!d_symbolRegex.isPrepared()) {
        if (prepare(&d_symbolRegex, k_SYMBOL_PATTERN)) {
            return 2;
        }
    }

    d_subject = subject;
    d_offset  = 0;
    d_position.reset(subject);
    d_extra.kind = LexerToken::e_INVALID;  // "not in use"
    return 0;
}

int Lexer::next(LexerToken* token) {
    BSLS_ASSERT(token);
    BSLS_ASSERT(d_tokenRegex.isPrepared());
    BSLS_ASSERT(d_symbolRegex.isPrepared());

    // already have one ready from previous call to 'next'
    if (d_extra.kind != LexerToken::e_INVALID) {
        *token = d_extra;

        // un-extra the token, unless it's EOF. EOF we'll keep giving.
        if (d_extra.kind != LexerToken::e_EOF) {
            d_extra.kind = LexerToken::e_INVALID;
        }

        return 0;
    }

    int rc = d_tokenRegex.match(
        &d_results, d_subject.data(), d_subject.size(), d_offset);
    if (rc == 0) {
        // The match succeeded, so the subpattern's .offset and .kind are going
        // to be important, either now or later. Store info in 'd_extra' for
        // now.
        BSLS_ASSERT(d_results.size() ==
                    bdlb::ArrayUtil::size(k_subpatterns) + 1);

        d_extra.text = bsl::string_view(d_subject.data() + d_results[0].first,
                                        d_results[0].second);

        d_extra.offset = d_results[0].first;

        const MatchIter found =
            std::find_if(d_results.begin() + 1, d_results.end(), &validOffset);

        BSLS_ASSERT(found != d_results.end());

        d_extra.kind = k_subpatterns[found - (d_results.begin() + 1)];
        // line and column information will be filled in later
    }
    else {
        // The match didn't succeed, so either we'll fail entirely, or will
        // next emit an EOF. Store info in 'd_extra' for now.
        d_extra.text   = bsl::string_view();
        d_extra.offset = d_subject.size();
        d_extra.kind   = LexerToken::e_EOF;
        // line and column information will be filled in later
    }

    // If the matching (or non-matching) offset is where we ended up last time,
    // then there's no gap, and we can emit the token that we just scanned.
    if (d_extra.offset == d_offset) {
        d_extra.beginLine   = d_position.line();
        d_extra.beginColumn = d_position.column();

        d_offset += d_extra.text.size();
        d_position.advanceToOffset(d_subject, d_offset);

        d_extra.endLine   = d_position.line();
        d_extra.endColumn = d_position.column();

        *token       = d_extra;
        d_extra.kind = LexerToken::e_INVALID;
        return 0;
    }

    // The offset is beyond where we started. Either we'll scan a symbol
    // spanning the gap exactly, or the input (subject) is invalid.
    const bsl::size_t                   gapSize = d_extra.offset - d_offset;
    bsl::pair<bsl::size_t, bsl::size_t> result;
    rc = d_symbolRegex.match(
        &result, d_subject.data() + d_offset, d_subject.size() - d_offset);
    if (rc || !(result.first == 0 && result.second == gapSize)) {
        // Either we didn't find a symbol in the gap, or it didn't fill the
        // entire gap. Either way, that's invalid input.
        d_extra.kind = LexerToken::e_INVALID;
        return 1;
    }

    // A symbol fits in the gap. Calculate the begin/end line/column for the
    // symbol token and emit it. Then calculate the begin/end line/column
    // for the extra token and keep it for the next call to 'next'.
    token->text   = bsl::string_view(d_subject.data() + d_offset, gapSize);
    token->offset = d_offset;
    token->kind   = LexerToken::e_SYMBOL;

    token->beginLine   = d_position.line();
    token->beginColumn = d_position.column();

    d_offset += gapSize;
    d_position.advanceToOffset(d_subject, d_offset);

    token->endLine = d_extra.beginLine = d_position.line();
    token->endColumn = d_extra.beginColumn = d_position.column();

    d_offset += d_extra.text.size();
    d_position.advanceToOffset(d_subject, d_offset);

    d_extra.endLine   = d_position.line();
    d_extra.endColumn = d_position.column();

    return 0;
}

}  // namespace lspcore
