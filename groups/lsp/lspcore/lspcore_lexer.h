#ifndef INCLUDED_LSPCORE_LEXER
#define INCLUDED_LSPCORE_LEXER

#include <bdlpcre_regex.h>
#include <bsl_cstddef.h>
#include <bsl_iosfwd.h>
#include <bsl_string_view.h>
#include <bsl_utility.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <lspcore_linecounter.h>

namespace lspcore {
namespace bdlpcre = BloombergLP::bdlpcre;
namespace bslma = BloombergLP::bslma;
namespace bslstl = BloombergLP::bslstl;

struct LexerToken {
    // TODO: document

    enum Kind {
        e_WHITESPACE,
        e_EOF,
        e_TRUE,
        e_FALSE,
        e_STRING,
        e_BYTES,
        e_DOUBLE,
        e_DECIMAL64,
        e_INT32,
        e_INT64,
        e_SYMBOL,
        e_OPEN_PARENTHESIS,
        e_CLOSE_PARENTHESIS,
        e_OPEN_SQUARE_BRACKET,
        e_CLOSE_SQUARE_BRACKET,
        e_OPEN_CURLY_BRACE,
        e_CLOSE_CURLY_BRACE,
        e_QUOTE,
        e_QUASIQUOTE,
        e_UNQUOTE,
        e_UNQUOTE_SPLICING,
        e_SYNTAX,
        e_QUASISYNTAX,
        e_UNSYNTAX,
        e_UNSYNTAX_SPLICING,
        e_COMMENT_LINE,
        e_COMMENT_DATUM,
        e_COMMENT_SHEBANG,
        e_DATE,
        e_TIME,
        e_DATETIME,
        e_DATETIME_INTERVAL,
        e_ERROR_TAG,
        e_USER_DEFINED_TYPE_TAG,
        e_PAIR_SEPARATOR
    };

    static const char *toAscii(Kind kind);
        // Return a pointer to a null-terminated string containing the name of
        // the specified token 'kind'. The behavior is undefined unless 'kind'
        // is one of the enumerated values.

    Kind kind;
    bsl::string_view text;
    bsl::size_t offset; // from the beginning of the input

    // Lines and columns are numbered starting at one, with the exception of
    // newline characters, which are considered to occupy column zero.
    // '(beginLine, beginColumn)' refers to the first character of 'text'.
    // '(endLine, endColumn)' refers to the character following the last
    // character of 'text'.
    bsl::size_t beginLine;
    bsl::size_t beginColumn;
    bsl::size_t endLine;
    bsl::size_t endColumn;
};

bsl::ostream& operator<<(bsl::ostream& stream, const LexerToken& token);
    // Insert the specified 'token' into the specified 'stream'. Return a
    // reference providing modifiable access to 'stream'. Note that the format
    // is unspecified and is intended for use in debugging.

class Lexer {
    // TODO: document

    bsl::vector<bsl::pair<bsl::size_t, bsl::size_t> > d_results;
    bdlpcre::RegEx d_tokenRegex;
    bdlpcre::RegEx d_symbolRegex;
    bsl::string_view d_subject;
    bsl::size_t d_offset;
    LineCounter d_position;
    LexerToken d_extra; // in case the previous 'next' found two tokens

  public:
    explicit Lexer(bslma::Allocator *allocator = 0);
        // TODO: document

    int reset(bsl::string_view subject);
        // TODO: document
        // TODO: in particular, that the data referred to by 'subject' must
        // live until either this object is destroyed or reset is called again.

    int next(LexerToken *token);
        // Scan the next token in the current subject. On success, assign the
        // scanned token through the specified 'token' and return zero. If
        // there is no more input, then assign a token of kind 'e_EOF'. If
        // input remains but a token cannot be scanned, return a nonzero value
        // without assigning through 'token'.

    bsl::size_t offset() const;
        // TODO: document

    bsl::size_t line() const;
        // TODO: document

    bsl::size_t column() const;
        // TODO: document
};

}  // close namespace lspcore

#endif
