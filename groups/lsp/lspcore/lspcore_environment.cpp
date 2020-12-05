#include <bsl_utility.h>
#include <bslstl_stringref.h>
#include <lspcore_environment.h>

using namespace BloombergLP;

namespace lspcore {

Environment::Environment(bslma::Allocator* allocator)
: d_locals(allocator)
, d_parent_p(0) {
}

Environment::Environment(Environment* parent, bslma::Allocator* allocator)
: d_locals(allocator)
, d_parent_p(parent) {
}

const bsl::pair<const bsl::string, bdld::Datum>* Environment::lookup(
    bsl::string_view name) const {
    // TODO: The call to 'find' would not compile with just 'name', I needed to
    // wrap it in 'StringRef'. The error was that something wasn't
    // transparent, and then no conversion from 'string_view' to 'const
    // string&'. I'm guessing that the transparency is to allow looking up by a
    // not-exactly-the-same-type, which is what I want, so I'm not sure what
    // went wrong there. I suspect that this code is generating a 'string' on
    // the fly with every lookup, which is bad. Look into this.
    const Environment* env = this;
    do {
        const bsl::unordered_map<bsl::string, bdld::Datum>::const_iterator
            found = env->d_locals.find(bslstl::StringRef(name));

        if (found != env->d_locals.end()) {
            return &*found;
        }
    } while ((env = env->d_parent_p));

    return 0;
}

bsl::pair<const bsl::string, bdld::Datum>* Environment::lookup(
    bsl::string_view name) {
    return const_cast<bsl::pair<const bsl::string, bdld::Datum>*>(
        static_cast<const Environment*>(this)->lookup(name));
}

bsl::pair<const bsl::string, bdld::Datum>* Environment::define(
    bsl::string_view name, const bdld::Datum& value) {
    typedef bsl::pair<bsl::unordered_map<bsl::string, bdld::Datum>::iterator,
                      bool>
        InsertResult;

    const InsertResult result = d_locals.insert(
        bsl::pair<const bsl::string, bdld::Datum>(name, value));

    if (result.second) {
        // inserted it
        return &*result.first;
    }

    // it was already there
    return 0;
}

}  // namespace lspcore
