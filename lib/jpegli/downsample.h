// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_PDFCORE_DOWNSAMPLE_H_
#define PDFCORE_LIB_PDFCORE_DOWNSAMPLE_H_

#include "lib/jpegli/common.h"

namespace pdfcore {

void ChooseDownsampleMethods(j_compress_ptr cinfo);

void DownsampleInputBuffer(j_compress_ptr cinfo);

void ApplyInputSmoothing(j_compress_ptr cinfo);

}  // namespace pdfcore

#endif  // PDFCORE_LIB_PDFCORE_DOWNSAMPLE_H_
