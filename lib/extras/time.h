// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_EXTRAS_TIME_H_
#define PDFCORE_LIB_EXTRAS_TIME_H_

// OS-specific function for timing.

namespace pdfcore {

// Returns current time [seconds] from a monotonic clock with unspecified
// starting point - only suitable for computing elapsed time.
double Now();

}  // namespace pdfcore

#endif  // PDFCORE_LIB_EXTRAS_TIME_H_
