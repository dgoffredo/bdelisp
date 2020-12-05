#ifndef INCLUDED_LSPCORE_NATIVEPROCEDUREUTIL
#define INCLUDED_LSPCORE_NATIVEPROCEDUREUTIL

#include <bdld_datum.h>
#include <bdld_datumudt.h>
#include <bsl_cstddef.h>
#include <bsl_functional.h>
#include <bsl_vector.h>
#include <bsls_assert.h>
#include <lspcore_endian.h>
#include <lspcore_userdefinedtypes.h>

namespace BloombergLP {
namespace bslma {
class Allocator;
}  // namespace bslma
}  // namespace BloombergLP

namespace lspcore {
namespace bdld  = BloombergLP::bdld;
namespace bslma = BloombergLP::bslma;

class Environment;

class NativeProcedureUtil {
  public:
    typedef void(Signature)(
        // 'argsAndOutput' contains the arguments passed to the procedure. The
        // procedure returns a value by resizing 'argsAndOutput' to size 1 and
        // assigned the return value to the vector's first element.
        bsl::vector<bdld::Datum>* argsAndOutput,
        // 'environment' is the lexical environment in which the procedure is
        // being invoked. If during the course of its execution, the procedure
        // produces a procedure that captures part of the environment, the
        // executing procedure must call 'environment.markAsReferenced()'.
        Environment& environment,
        // All parts of the resulting 'bdld::Datum' must be allocated using
        // 'allocator'.
        bslma::Allocator* allocator);

    static bdld::Datum create(const bsl::function<Signature>&,
                              bslma::Allocator*);
    static bdld::Datum create(Signature*, bslma::Allocator*);

    static void invoke(const bdld::Datum&        function,
                       bsl::vector<bdld::Datum>* argsAndOutput,
                       Environment&              environment,
                       bslma::Allocator*         allocator);

    static void invoke(const bdld::DatumUdt&     function,
                       bsl::vector<bdld::Datum>* argsAndOutput,
                       Environment&              environment,
                       bslma::Allocator*         allocator);

    static void invoke(void*                     datumUdtData,
                       bsl::vector<bdld::Datum>* argsAndOutput,
                       Environment&              environment,
                       bslma::Allocator*         allocator);

  private:
    static const bool FUNC_PTR_FITS = sizeof(Signature*) == sizeof(void*);
};

bdld::Datum NativeProcedureUtil::create(
    const bsl::function<Signature>& function, bslma::Allocator* allocator) {
    BSLS_ASSERT(allocator);

    // Using allocators in 'bsl::function' is ugly. The corresponding facility
    // in 'std::function' is deprecated as of C++17. I don't expect that BDE,
    // allocator stalwart that it is, will remove the feature.
    bsl::function<Signature>* copy = new (*allocator) bsl::function<Signature>(
        bsl::allocator_arg,
        bsl::allocator<bsl::function<Signature> >(allocator),
        function);

    // The 'DatumUdt.data()' pointer will be 'copy', except that the lowest bit
    // of its lowest byte will be set to indicate that it's a pointer to an
    // owned 'bsl::function', rather than a pointer to a plain function.
    void* data   = copy;
    char* buffer = reinterpret_cast<char*>(&data);
    LSPCORE_LOWBYTE(buffer) |= 1;

    return bdld::Datum::createUdt(data, UserDefinedTypes::e_NATIVE_PROCEDURE);
}

bdld::Datum NativeProcedureUtil::create(Signature*        function,
                                        bslma::Allocator* allocator) {
    void* data;

    if (FUNC_PTR_FITS) {
        data = reinterpret_cast<void*>(function);
    }
    else {
        // I haven't worked with any platforms where this is the case, but here
        // we go.
        data = new (*allocator) Signature*(function);
    }

    return bdld::Datum::createUdt(data, UserDefinedTypes::e_NATIVE_PROCEDURE);
}

void NativeProcedureUtil::invoke(const bdld::Datum&        function,
                                 bsl::vector<bdld::Datum>* argsAndOutput,
                                 Environment&              environment,
                                 bslma::Allocator*         allocator) {
    BSLS_ASSERT(function.isUdt());
    return invoke(function.theUdt(), argsAndOutput, environment, allocator);
}

void NativeProcedureUtil::invoke(const bdld::DatumUdt&     function,
                                 bsl::vector<bdld::Datum>* argsAndOutput,
                                 Environment&              environment,
                                 bslma::Allocator*         allocator) {
    return invoke(function.data(), argsAndOutput, environment, allocator);
}

void NativeProcedureUtil::invoke(void*                     datumUdtData,
                                 bsl::vector<bdld::Datum>* argsAndOutput,
                                 Environment&              environment,
                                 bslma::Allocator*         allocator) {
    // Unpack the invokable from 'datumUdtData', and then invoke it with the
    // specified arguments.

    char* buffer = reinterpret_cast<char*>(&datumUdtData);
    if (LSPCORE_LOWBYTE(buffer) & 1) {
        // The lowest bit of the pointer is set. This means that it's a pointer
        // to a 'bsl::function' (once we've cleared that lowest bit).
        void* ptr = reinterpret_cast<void*>(
            reinterpret_cast<bsl::uintptr_t>(datumUdtData) &
            ~bsl::uintptr_t(1));
        bsl::function<Signature>* function =
            static_cast<bsl::function<Signature>*>(ptr);
        return (*function)(argsAndOutput, environment, allocator);
    }

    // The lowest bit of the pointer is not set. This means that it's either a
    // function pointer, or a pointer to a function pointer, depending on
    // whether data pointers and function pointers have the same size.
    if (FUNC_PTR_FITS) {
        Signature* function = reinterpret_cast<Signature*>(datumUdtData);
        return function(argsAndOutput, environment, allocator);
    }

    // I haven't worked with any platforms where this is the case, but here we
    // go.
    Signature** functionPtrPtr = static_cast<Signature**>(datumUdtData);
    return (*functionPtrPtr)(argsAndOutput, environment, allocator);
}

}  // namespace lspcore

#endif
