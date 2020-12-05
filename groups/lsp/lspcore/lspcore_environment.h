#ifndef INCLUDED_LSPCORE_ENVIRONMENT
#define INCLUDED_LSPCORE_ENVIRONMENT

#include <bdld_datum.h>
#include <bsl_string_view.h>
#include <bsl_unordered_map.h>

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

class Environment;

class Environment {
    bsl::unordered_map<bsl::string, bdld::Datum> d_locals;
    Environment*                                 d_parent_p;
    // 'd_wasReferenced' is used by an optimization in the interpreter. When a
    // new environment is created that references this object as its parent, we
    // set 'd_wasReferenced = true'. Then when evaluating tail calls, the
    // interpreter has the option of reusing this environment instead of
    // creating a new one, provided that 'd_wasReferenced == false'.
    bool d_wasReferenced;

  public:
    explicit Environment(bslma::Allocator*);
    explicit Environment(Environment* parent, bslma::Allocator*);

    // Return a pointer to the environment entry having the specified 'name',
    // or return null if there is no such entry.
    bsl::pair<const bsl::string, bdld::Datum>* lookup(bsl::string_view name);
    const bsl::pair<const bsl::string, bdld::Datum>* lookup(
        bsl::string_view name) const;

    // In the environment local to this object, create an entry having the
    // specified 'name' and the specified 'value' if one does not already exist
    // in the local environment. Return a pointer to the created entry, or
    // return null if an entry already exists with that name in the local
    // environment. The local environment consists of those names stored within
    // this object specifically, without considering the ancestors of this
    // object.
    bsl::pair<const bsl::string, bdld::Datum>* define(
        bsl::string_view name, const bdld::Datum& value);

    // Return whether this object has ever been the parent of another
    // environment.
    bool wasReferenced() const;

    void markAsReferenced();

    bslma::Allocator* allocator() const;
};

inline bslma::Allocator* Environment::allocator() const {
    return d_locals.get_allocator().mechanism();
}

inline bool Environment::wasReferenced() const {
    return d_wasReferenced;
}

inline void Environment::markAsReferenced() {
    d_wasReferenced = true;
}

}  // namespace lspcore

#endif
