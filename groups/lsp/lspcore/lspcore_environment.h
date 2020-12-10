#ifndef INCLUDED_LSPCORE_ENVIRONMENT
#define INCLUDED_LSPCORE_ENVIRONMENT

#include <bdld_datum.h>
#include <bsl_string_view.h>
#include <bsl_unordered_map.h>
#include <bsl_utility.h>
#include <bsl_vector.h>

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

// 'struct StringyHash' and 'struct StringyEqualTo', below, are used in the
// 'unordered_map' type within each 'Environment'. These helpers allow the
// 'unordered_map' to store 'bsl::string' keys while supporting lookups using
// 'bsl::string_view', without a temporary 'bsl::string' having to be
// constructed for each lookup.

struct StringyHash : public bsl::hash<bsl::string_view> {
    typedef void is_transparent;
};

struct StringyEqualTo : public bsl::equal_to<bsl::string_view> {
    typedef void is_transparent;
};

class Environment {
    bsl::vector<bsl::pair<const bsl::string, bdld::Datum>*> d_arguments;

    bsl::unordered_map<bsl::string, bdld::Datum, StringyHash, StringyEqualTo>
        d_locals;

    Environment* d_parent_p;

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

    bsl::vector<bsl::pair<const bsl::string, bdld::Datum>*>&       arguments();
    const bsl::vector<bsl::pair<const bsl::string, bdld::Datum>*>& arguments()
        const;

    // In the environment local to this object, create an entry having the
    // specified 'name' and the specified 'value' if one does not already exist
    // in the local environment. Return a pointer to the entry, and a 'bool'
    // indicating whether the entry was inserted (as opposed to already having
    // been present). The local environment consists of those names stored
    // within this object specifically, without considering the ancestors of
    // this object.
    bsl::pair<bsl::pair<const bsl::string, bdld::Datum>*, bool> define(
        bsl::string_view name, const bdld::Datum& value);

    // Call 'define', but additionally modify the local binding if it already
    // exists. Return a pointer to the relevant entry.
    bsl::pair<const bsl::string, bdld::Datum>* defineOrRedefine(
        bsl::string_view name, const bdld::Datum& value);

    // Remove all local entries from this environment.
    void clearLocals();

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

inline bsl::vector<bsl::pair<const bsl::string, bdld::Datum>*>&
Environment::arguments() {
    return d_arguments;
}

inline const bsl::vector<bsl::pair<const bsl::string, bdld::Datum>*>&
Environment::arguments() const {
    return d_arguments;
}

}  // namespace lspcore

#endif
