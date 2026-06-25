/* Copyright (c) the JPEG XL Project Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/** Implementation of PdfcoreParallelRunner than can be used to enable
 * multithreading when using the JPEGLI library. This uses std::thread
 * internally and related synchronization functions. The number of threads
 * created is fixed at construction time and the threads are re-used for every
 * ThreadParallelRunner::Runner call. Only one concurrent
 * PdfcoreThreadParallelRunner call per instance is allowed at a time.
 *
 * This is a scalable, lower-overhead thread pool runner, especially suitable
 * for data-parallel computations in the fork-join model, where clients need to
 * know when all tasks have completed.
 *
 * This thread pool can efficiently load-balance millions of tasks using an
 * atomic counter, thus avoiding per-task virtual or system calls. With 48
 * hyperthreads and 1M tasks that add to an atomic counter, overall runtime is
 * 10-20x higher when using std::async, and ~200x for a queue-based thread
 */

#ifndef PDFCORE_THREAD_PARALLEL_RUNNER_H_
#define PDFCORE_THREAD_PARALLEL_RUNNER_H_

#include <jpegli/jpegli_threads_export.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "lib/base/memory_manager.h"
#include "lib/base/parallel_runner.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Parallel runner internally using std::thread. Use as @ref
 * PdfcoreParallelRunner.
 */
PDFCORE_THREADS_EXPORT PdfcoreParallelRetCode PdfcoreThreadParallelRunner(
    void* runner_opaque, void* pdfcore_jpegli_opaque, PdfcoreParallelRunInit init,
    PdfcoreParallelRunFunction func, uint32_t start_range, uint32_t end_range);

/** Creates the runner for @ref PdfcoreThreadParallelRunner. Use as the opaque
 * runner.
 */
PDFCORE_THREADS_EXPORT void* PdfcoreThreadParallelRunnerCreate(
    const PdfcoreMemoryManager* memory_manager, size_t num_worker_threads);

/** Destroys the runner created by @ref PdfcoreThreadParallelRunnerCreate.
 */
PDFCORE_THREADS_EXPORT void PdfcoreThreadParallelRunnerDestroy(
    void* runner_opaque);

/** Returns a default num_worker_threads value for
 * @ref PdfcoreThreadParallelRunnerCreate.
 */
PDFCORE_THREADS_EXPORT size_t
PdfcoreThreadParallelRunnerDefaultNumWorkerThreads(void);

#ifdef __cplusplus
}
#endif

#endif /* PDFCORE_THREAD_PARALLEL_RUNNER_H_ */
