#ifndef INCLUDED_LSPCORE_LINECOUNTER
#define INCLUDED_LSPCORE_LINECOUNTER

#include <bsl_algorithm.h>
#include <bsl_cstddef.h>
#include <bsl_string_view.h>

namespace lspcore {

class LineCounter {
    bsl::size_t d_offset;
    bsl::size_t d_line;
    bsl::size_t d_column;

  public:
    // TODO peens peens peens peens
    void reset(bsl::string_view string);

    // TODO peens peens peens peens
    void advanceToOffset(bsl::string_view string, bsl::size_t absoluteOffset);

    bsl::size_t offset();
    bsl::size_t line();
    bsl::size_t column();
};

inline bsl::size_t LineCounter::offset() {
    return d_offset;
}

inline bsl::size_t LineCounter::line() {
    return d_line;
}

inline bsl::size_t LineCounter::column() {
    return d_column;
}

}  // namespace lspcore

#endif
