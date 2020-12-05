#ifndef INCLUDED_LSPCORE_ENDIAN
#define INCLUDED_LSPCORE_ENDIAN

#include <bsls_platform.h>

// TODO: BDE might have utilities for these already. Also, consider using
// inline functions in an 'impl' or 'detail' namespace, instead of using
// macros.
#ifdef BSLS_PLATFORM_IS_LITTLE_ENDIAN
// little-endian, i.e. the least significant byte is first in memory
#define LSPCORE_LOWBYTE(BUFFER) ((BUFFER)[0])
// 'LSPCORE_BEGIN' is the beginning of the in-place string destination.
#define LSPCORE_BEGIN(BUFFER) ((BUFFER) + 1)
#else  // assume big-endian (ignore the possibility of mixed-endian systems)
// big-endian, i.e. the least significant byte is last in memory
#define LSPCORE_LOWBYTE(BUFFER) ((BUFFER)[sizeof(void*) - 1])
// 'LSPCORE_BEGIN' is the beginning of the in-place string destination.
#define LSPCORE_BEGIN(BUFFER)   (BUFFER)
#endif

#endif
