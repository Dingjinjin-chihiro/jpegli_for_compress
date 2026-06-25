// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_PDFCORE_UPSAMPLE_H_
#define PDFCORE_LIB_PDFCORE_UPSAMPLE_H_

#include <stddef.h>

#include "lib/base/compiler_specific.h"

namespace pdfcore {

void Upsample2Horizontal(float* PDFCORE_RESTRICT row,
                         float* PDFCORE_RESTRICT scratch_space, size_t len_out);

void Upsample2Vertical(const float* PDFCORE_RESTRICT row_top,
                       const float* PDFCORE_RESTRICT row_mid,
                       const float* PDFCORE_RESTRICT row_bot,
                       float* PDFCORE_RESTRICT row_out0,
                       float* PDFCORE_RESTRICT row_out1, size_t len);

}  // namespace pdfcore

#endif  // PDFCORE_LIB_PDFCORE_UPSAMPLE_H_
