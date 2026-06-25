// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// @addtogroup libpdfcore_jpegli_cpp
/// @{
///
/// @file thread_parallel_runner_cxx.h
/// @brief C++ header-only helper for @ref thread_parallel_runner.h.
///
/// There's no binary library associated with the header since this is a header
/// only library.

#ifndef PDFCORE_THREAD_PARALLEL_RUNNER_CXX_H_
#define PDFCORE_THREAD_PARALLEL_RUNNER_CXX_H_

#include <cstddef>
#include <memory>

#include "lib/base/memory_manager.h"
#include "lib/threads/thread_parallel_runner.h"

#ifndef __cplusplus
#error \
    "This a C++ only header. Use jpegli/pdfcore_jpegli_thread_parallel_runner.h from C" \
    "sources."
#endif

/// Struct to call PdfcoreThreadParallelRunnerDestroy from the
/// PdfcoreThreadParallelRunnerPtr unique_ptr.
struct PdfcoreThreadParallelRunnerDestroyStruct {
  /// Calls @ref PdfcoreThreadParallelRunnerDestroy() on the passed runner.
  void operator()(void* runner) { PdfcoreThreadParallelRunnerDestroy(runner); }
};

/// std::unique_ptr<> type that calls PdfcoreThreadParallelRunnerDestroy() when
/// releasing the runner.
///
/// Use this helper type from C++ sources to ensure the runner is destroyed and
/// their internal resources released.
typedef std::unique_ptr<void, PdfcoreThreadParallelRunnerDestroyStruct>
    PdfcoreThreadParallelRunnerPtr;

/// Creates an instance of PdfcoreThreadParallelRunner into a
/// PdfcoreThreadParallelRunnerPtr and initializes it.
///
/// This function returns a unique_ptr that will call
/// PdfcoreThreadParallelRunnerDestroy() when releasing the pointer. See @ref
/// PdfcoreThreadParallelRunnerCreate for details on the instance creation.
///
/// @param memory_manager custom allocator function. It may be NULL. The memory
///        manager will be copied internally.
/// @param num_worker_threads the number of worker threads to create.
/// @return a @c NULL PdfcoreThreadParallelRunnerPtr if the instance can not be
/// allocated or initialized
/// @return initialized PdfcoreThreadParallelRunnerPtr instance otherwise.
static inline PdfcoreThreadParallelRunnerPtr PdfcoreThreadParallelRunnerMake(
    const PdfcoreMemoryManager* memory_manager, size_t num_worker_threads) {
  return PdfcoreThreadParallelRunnerPtr(
      PdfcoreThreadParallelRunnerCreate(memory_manager, num_worker_threads));
}

#endif  // PDFCORE_THREAD_PARALLEL_RUNNER_CXX_H_

/// @}
