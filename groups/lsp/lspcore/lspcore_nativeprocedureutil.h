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

// A native procedure is a C++ function that we can call from within the
// interpreter. It's either a plain old function pointer, or a pointer to a
// managed 'bsl::function' object. The plain old function pointer can fit
// in the 'DatumUdt' 'data()' member, provided that function pointers and
// data pointers are the same size. On platforms (are there any?) where
// function pointers are larger, then we instead use a pointer to a function
// pointer. For the 'bsl::function' case, we store a pointer to a
// 'bsl::function'.
//
// These storage cases are distinguished from each other using the lowest
// bit of the lowest byte of the 'DatumUdt' 'data()' member. If that bit is
// set, then the pointer is a 'bsl::function*' (except for the low bit). If the
// bit is unset, then the pointer is a function pointer, or a pointer to a
// function pointer if a function pointer won't fit.

class NativeProcedureUtil {
  public:
    struct Arguments {
        // 'argsAndOutput' contains the arguments passed to the procedure. The
        // procedure returns a value by resizing 'argsAndOutput' to size 1 and
        // assigned the return value to the vector's first element. A native
        // procedure signals an error by raising an exception of type
        // 'bdld::Datum', where the datum object is of error type.
        // 'argsAndOutput' is never null.
        bsl::vector<bdld::Datum>* argsAndOutput;

        // 'environment' is the lexical environment in which the procedure is
        // being invoked. If during the course of its execution, the procedure
        // produces a procedure that captures part of the environment, the
        // executing procedure must call 'environment.markAsReferenced()'.
        // 'environment' is never null.
        Environment* environment;

        // To accomodate 'bdld::DatumUdt' types defined outside of this
        // library, each interpreter has a configurable 'typeOffset' that is
        // applied to the type ID of 'bdld::DatumUdt' types defined within
        // this library. This could be used to avoid collisions with types
        // defined elsewhere.
        int typeOffset;

        // All parts of the returned or thrown 'bdld::Datum' must be allocated
        // using 'allocator'.
        bslma::Allocator* allocator;
    };

    typedef void(Signature)(const Arguments&);

    static bool isNativeProcedure(const bdld::Datum& datum, int typeOffset);
    static bool isNativeProcedure(const bdld::DatumUdt& udt, int typeOffset);

    static bdld::Datum create(const bsl::function<Signature>&,
                              int typeOffset,
                              bslma::Allocator*);
    static bdld::Datum create(Signature*, int typeOffset, bslma::Allocator*);

    static void invoke(const bdld::Datum& function, const Arguments&);

    static void invoke(const bdld::DatumUdt& function, const Arguments&);

  private:
    static void invoke(void* datumUdtData, const Arguments&);

    static const bool FUNC_PTR_FITS = sizeof(Signature*) == sizeof(void*);
};

inline bool NativeProcedureUtil::isNativeProcedure(const bdld::Datum& datum,
                                                   int typeOffset) {
    return datum.isUdt() && isNativeProcedure(datum.theUdt(), typeOffset);
}

inline bool NativeProcedureUtil::isNativeProcedure(const bdld::DatumUdt& udt,
                                                   int typeOffset) {
    return udt.type() - typeOffset == UserDefinedTypes::e_NATIVE_PROCEDURE;
}

inline bdld::Datum NativeProcedureUtil::create(
    const bsl::function<Signature>& function,
    int                             typeOffset,
    bslma::Allocator*               allocator) {
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

    return bdld::Datum::createUdt(
        data, UserDefinedTypes::e_NATIVE_PROCEDURE + typeOffset);
}

inline bdld::Datum NativeProcedureUtil::create(Signature*        function,
                                               int               typeOffset,
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

    return bdld::Datum::createUdt(
        data, UserDefinedTypes::e_NATIVE_PROCEDURE + typeOffset);
}

inline void NativeProcedureUtil::invoke(const bdld::Datum& function,
                                        const Arguments&   arguments) {
    BSLS_ASSERT(function.isUdt());
    return invoke(function.theUdt(), arguments);
}

inline void NativeProcedureUtil::invoke(const bdld::DatumUdt& function,
                                        const Arguments&      arguments) {
    return invoke(function.data(), arguments);
}

inline void NativeProcedureUtil::invoke(void*            datumUdtData,
                                        const Arguments& arguments) {
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
        return (*function)(arguments);
    }

    // The lowest bit of the pointer is not set. This means that it's either a
    // function pointer, or a pointer to a function pointer, depending on
    // whether data pointers and function pointers have the same size.
    if (FUNC_PTR_FITS) {
        Signature* function = reinterpret_cast<Signature*>(datumUdtData);
        return function(arguments);
    }

    // I haven't worked with any platforms where this is the case, but here we
    // go.
    Signature** functionPtrPtr = static_cast<Signature**>(datumUdtData);
    return (*functionPtrPtr)(arguments);
}

}  // namespace lspcore

#endif
