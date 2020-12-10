#include <bsl_utility.h>
#include <bslstl_stringref.h>
#include <lspcore_environment.h>

using namespace BloombergLP;

namespace lspcore {

Environment::Environment(bslma::Allocator* allocator)
: d_locals(allocator)
, d_parent_p(0)
, d_wasReferenced(false) {
}

Environment::Environment(Environment* parent, bslma::Allocator* allocator)
: d_locals(allocator)
, d_parent_p(parent)
, d_wasReferenced(false) {
    BSLS_ASSERT(parent);
    parent->markAsReferenced();
}

const bsl::pair<const bsl::string, bdld::Datum>* Environment::lookup(
    bsl::string_view name) const {
    const Environment* env = this;
    do {
        const bsl::unordered_map<bsl::string, bdld::Datum>::const_iterator
            found = env->d_locals.find(name);

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

bsl::pair<bsl::pair<const bsl::string, bdld::Datum>*, bool>
Environment::define(bsl::string_view name, const bdld::Datum& value) {
    typedef bsl::pair<bsl::unordered_map<bsl::string, bdld::Datum>::iterator,
                      bool>
        InsertResult;

    const InsertResult result = d_locals.insert(
        bsl::pair<const bsl::string, bdld::Datum>(name, value));

    return bsl::make_pair(&*result.first, result.second);
}

bsl::pair<const bsl::string, bdld::Datum>* Environment::defineOrRedefine(
    bsl::string_view name, const bdld::Datum& value) {
    typedef bsl::pair<bsl::unordered_map<bsl::string, bdld::Datum>::iterator,
                      bool>
        InsertResult;

    const InsertResult result = d_locals.insert(
        bsl::pair<const bsl::string, bdld::Datum>(name, value));

    if (!result.second) {
        // There's already an entry at 'name'. Give it a new 'value'.
        result.first->second = value;
    }

    return &*result.first;
}

void Environment::clearLocals() {
    d_locals.clear();
}

}  // namespace lspcore
