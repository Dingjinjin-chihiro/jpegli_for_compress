// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_PDFCORE_SIMD_H_
#define PDFCORE_LIB_PDFCORE_SIMD_H_

#include <stddef.h>

namespace pdfcore {

// Returns SIMD vector size in bytes.
size_t VectorSize();

}  // namespace pdfcore

#endif  // PDFCORE_LIB_PDFCORE_SIMD_H_
