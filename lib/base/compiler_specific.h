// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_BASE_COMPILER_SPECIFIC_H_
#define PDFCORE_LIB_BASE_COMPILER_SPECIFIC_H_

// Macros for compiler version + nonstandard keywords, e.g. __builtin_expect.

#include <sys/types.h>  // IWYU pragma: export
#ifdef __clang_analyzer__
#include <stdio.h>  // IWYU pragma: export
#endif

#include "lib/base/sanitizer_definitions.h"

#if PDFCORE_ADDRESS_SANITIZER || PDFCORE_MEMORY_SANITIZER || \
    PDFCORE_THREAD_SANITIZER
#include "sanitizer/common_interface_defs.h"  // __sanitizer_print_stack_trace
#endif                                        // defined(*_SANITIZER)

// #if is shorter and safer than #ifdef. *_VERSION are zero if not detected,
// otherwise 100 * major + minor version. Note that other packages check for
// #ifdef COMPILER_MSVC, so we cannot use that same name.

#ifdef _MSC_VER
#define PDFCORE_COMPILER_MSVC _MSC_VER
#else
#define PDFCORE_COMPILER_MSVC 0
#endif

#ifdef __GNUC__
#define PDFCORE_COMPILER_GCC (__GNUC__ * 100 + __GNUC_MINOR__)
#else
#define PDFCORE_COMPILER_GCC 0
#endif

#ifdef __clang__
#define PDFCORE_COMPILER_CLANG (__clang_major__ * 100 + __clang_minor__)
// Clang pretends to be GCC for compatibility.
#undef PDFCORE_COMPILER_GCC
#define PDFCORE_COMPILER_GCC 0
#else
#define PDFCORE_COMPILER_CLANG 0
#endif

#if PDFCORE_COMPILER_MSVC
#define PDFCORE_RESTRICT __restrict
#elif PDFCORE_COMPILER_GCC || PDFCORE_COMPILER_CLANG
#define PDFCORE_RESTRICT __restrict__
#else
#define PDFCORE_RESTRICT
#endif

#if PDFCORE_COMPILER_MSVC
#define PDFCORE_INLINE __forceinline
#define PDFCORE_NOINLINE __declspec(noinline)
#else
#define PDFCORE_INLINE inline __attribute__((always_inline))
#define PDFCORE_NOINLINE __attribute__((noinline))
#endif

#if PDFCORE_COMPILER_MSVC
#define PDFCORE_NORETURN __declspec(noreturn)
#elif PDFCORE_COMPILER_GCC || PDFCORE_COMPILER_CLANG
#define PDFCORE_NORETURN __attribute__((noreturn))
#else
#define PDFCORE_NORETURN
#endif

#if PDFCORE_COMPILER_MSVC
#define PDFCORE_MAYBE_UNUSED
#else
// Encountered "attribute list cannot appear here" when using the C++17
// [[maybe_unused]], so only use the old style attribute for now.
#define PDFCORE_MAYBE_UNUSED __attribute__((unused))
#endif

// MSAN execution won't hurt if some code it not inlined, but this can greatly
// improve compilation time. Unfortunately this macro can not be used just
// everywhere - inside header files it leads to "multiple definition" error;
// though it would be better not to have PDFCORE_INLINE in header overall.
#if PDFCORE_MEMORY_SANITIZER || PDFCORE_ADDRESS_SANITIZER || \
    PDFCORE_THREAD_SANITIZER
#define PDFCORE_MAYBE_INLINE PDFCORE_MAYBE_UNUSED
#else
#define PDFCORE_MAYBE_INLINE PDFCORE_INLINE
#endif

#if PDFCORE_COMPILER_MSVC
// Unsupported, __assume is not the same.
#define PDFCORE_LIKELY(expr) expr
#define PDFCORE_UNLIKELY(expr) expr
#else
#define PDFCORE_LIKELY(expr) __builtin_expect(!!(expr), 1)
#define PDFCORE_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#endif

// Returns a void* pointer which the compiler then assumes is N-byte aligned.
// Example: float* PDFCORE_RESTRICT aligned = (float*)PDFCORE_ASSUME_ALIGNED(in,
// 32);
//
// The assignment semantics are required by GCC/Clang. ICC provides an in-place
// __assume_aligned, whereas MSVC's __assume appears unsuitable.
#if PDFCORE_COMPILER_CLANG
// Early versions of Clang did not support __builtin_assume_aligned.
#define PDFCORE_HAS_ASSUME_ALIGNED __has_builtin(__builtin_assume_aligned)
#elif PDFCORE_COMPILER_GCC
#define PDFCORE_HAS_ASSUME_ALIGNED 1
#else
#define PDFCORE_HAS_ASSUME_ALIGNED 0
#endif

#if PDFCORE_HAS_ASSUME_ALIGNED
#define PDFCORE_ASSUME_ALIGNED(ptr, align) \
  __builtin_assume_aligned((ptr), (align))
#else
#define PDFCORE_ASSUME_ALIGNED(ptr, align) (ptr) /* not supported */
#endif

#ifdef __has_attribute
#define PDFCORE_HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define PDFCORE_HAVE_ATTRIBUTE(x) 0
#endif

// Raises warnings if the function return value is unused. Should appear as the
// first part of a function definition/declaration.
#if PDFCORE_HAVE_ATTRIBUTE(nodiscard)
#define PDFCORE_MUST_USE_RESULT [[nodiscard]]
#elif PDFCORE_COMPILER_CLANG && PDFCORE_HAVE_ATTRIBUTE(warn_unused_result)
#define PDFCORE_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define PDFCORE_MUST_USE_RESULT
#endif

// Disable certain -fsanitize flags for functions that are expected to include
// things like unsigned integer overflow. For example use in the function
// declaration PDFCORE_NO_SANITIZE("unsigned-integer-overflow") to silence
// unsigned integer overflow ubsan messages.
#if PDFCORE_COMPILER_CLANG && PDFCORE_HAVE_ATTRIBUTE(no_sanitize)
#define PDFCORE_NO_SANITIZE(X) __attribute__((no_sanitize(X)))
#else
#define PDFCORE_NO_SANITIZE(X)
#endif

#if PDFCORE_HAVE_ATTRIBUTE(__format__)
#define PDFCORE_FORMAT(idx_fmt, idx_arg) \
  __attribute__((__format__(__printf__, idx_fmt, idx_arg)))
#else
#define PDFCORE_FORMAT(idx_fmt, idx_arg)
#endif

// C++ standard.
#if defined(_MSC_VER) && !defined(__clang__) && defined(_MSVC_LANG) && \
    _MSVC_LANG > __cplusplus
#define PDFCORE_CXX_LANG _MSVC_LANG
#else
#define PDFCORE_CXX_LANG __cplusplus
#endif

// Known / distinguished C++ standards.
#define PDFCORE_CXX_17 201703

// In most cases we consider build as "debug". Use `NDEBUG` for release build.
#if defined(PDFCORE_IS_DEBUG_BUILD)
#undef PDFCORE_IS_DEBUG_BUILD
#define PDFCORE_IS_DEBUG_BUILD 1
#elif defined(NDEBUG)
#define PDFCORE_IS_DEBUG_BUILD 0
#else
#define PDFCORE_IS_DEBUG_BUILD 1
#endif

#if defined(PDFCORE_CRASH_ON_ERROR)
#undef PDFCORE_CRASH_ON_ERROR
#define PDFCORE_CRASH_ON_ERROR 1
#else
#define PDFCORE_CRASH_ON_ERROR 0
#endif

#if PDFCORE_CRASH_ON_ERROR && !PDFCORE_IS_DEBUG_BUILD
#error "PDFCORE_CRASH_ON_ERROR requires PDFCORE_IS_DEBUG_BUILD"
#endif

// Pass -DPDFCORE_DEBUG_ON_ALL_ERROR at compile time to print debug messages on
// all error (fatal and non-fatal) status.
#if defined(PDFCORE_DEBUG_ON_ALL_ERROR)
#undef PDFCORE_DEBUG_ON_ALL_ERROR
#define PDFCORE_DEBUG_ON_ALL_ERROR 1
#else
#define PDFCORE_DEBUG_ON_ALL_ERROR 0
#endif

#if PDFCORE_DEBUG_ON_ALL_ERROR && !PDFCORE_IS_DEBUG_BUILD
#error "PDFCORE_DEBUG_ON_ALL_ERROR requires PDFCORE_IS_DEBUG_BUILD"
#endif

// Pass -DPDFCORE_DEBUG_ON_ABORT={0} to disable the debug messages on
// (debug) PDFCORE_ENSURE and PDFCORE_DASSERT.
#if !defined(PDFCORE_DEBUG_ON_ABORT)
#define PDFCORE_DEBUG_ON_ABORT PDFCORE_IS_DEBUG_BUILD
#endif  // PDFCORE_DEBUG_ON_ABORT

#if PDFCORE_DEBUG_ON_ABORT && !PDFCORE_IS_DEBUG_BUILD
#error "PDFCORE_DEBUG_ON_ABORT requires PDFCORE_IS_DEBUG_BUILD"
#endif

#if PDFCORE_ADDRESS_SANITIZER || PDFCORE_MEMORY_SANITIZER || \
    PDFCORE_THREAD_SANITIZER
#define PDFCORE_PRINT_STACK_TRACE() __sanitizer_print_stack_trace();
#else
#define PDFCORE_PRINT_STACK_TRACE()
#endif

#if PDFCORE_COMPILER_MSVC
#define PDFCORE_CRASH() __debugbreak(), (void)abort()
#else
#define PDFCORE_CRASH() (void)__builtin_trap()
#endif

#endif  // PDFCORE_LIB_BASE_COMPILER_SPECIFIC_H_
