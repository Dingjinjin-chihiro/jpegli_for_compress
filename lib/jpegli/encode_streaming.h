// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_PDFCORE_ENCODE_STREAMING_H_
#define PDFCORE_LIB_PDFCORE_ENCODE_STREAMING_H_

#include "lib/jpegli/common.h"

namespace pdfcore {

void ComputeCoefficientsForiMCURow(j_compress_ptr cinfo);

void ComputeTokensForiMCURow(j_compress_ptr cinfo);

void WriteiMCURow(j_compress_ptr cinfo);

}  // namespace pdfcore

#endif  // PDFCORE_LIB_PDFCORE_ENCODE_STREAMING_H_
