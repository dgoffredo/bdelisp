#include <lspcore_linecounter.h>

namespace lspcore {

void LineCounter::reset(bsl::string_view string) {
    d_offset = 0;
    if (string.empty()) {
        d_line   = 1;
        d_column = 0;
    }
    else {
        d_line   = string[0] == '\n' ? 2 : 1;
        d_column = string[0] == '\n' ? 0 : 1;
    }
}

void LineCounter::advanceToOffset(bsl::string_view string,
                                  bsl::size_t      absoluteOffset) {
    const char* last = string.data() + absoluteOffset;
    BSLS_ASSERT(last <= string.end());

    const char* first = string.data() + d_offset;
    if (first == last) {
        return;
    }

    if (last == string.end()) {
        BSLS_ASSERT(absoluteOffset != 0);
        advanceToOffset(string, absoluteOffset - 1);
        ++d_offset;
        ++d_column;
        return;
    }

    d_offset = absoluteOffset;

    const bsl::size_t lastNewline =
        bsl::string_view(first + 1, last - first).rfind('\n');

    if (lastNewline == bsl::string_view::npos) {
        d_column += last - first;
        return;
    }

    const bsl::size_t numNewlines =
        1 + bsl::count(first + 1, first + 1 + lastNewline, '\n');

    d_line += numNewlines;
    d_column = last - (first + 1 + lastNewline);
}

}  // namespace lspcore
