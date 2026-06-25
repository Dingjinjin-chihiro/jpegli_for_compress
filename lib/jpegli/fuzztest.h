// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef PDFCORE_LIB_PDFCORE_FUZZTEST_H_
#define PDFCORE_LIB_PDFCORE_FUZZTEST_H_

#include "lib/base/compiler_specific.h"

#if !defined(FUZZ_TEST)
struct FuzzTestSink {
  template <typename F>
  FuzzTestSink WithSeeds(F /*f*/) {
    return *this;
  }
};
#define FUZZ_TEST(A, B) \
  const PDFCORE_MAYBE_UNUSED FuzzTestSink unused##A##B = FuzzTestSink()
#endif

#endif  // PDFCORE_LIB_PDFCORE_FUZZTEST_H_
