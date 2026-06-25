// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_PDFCORE_COLOR_TRANSFORM_H_
#define PDFCORE_LIB_PDFCORE_COLOR_TRANSFORM_H_

#include "lib/base/compiler_specific.h"
#include "lib/jpegli/common.h"

namespace pdfcore {

void ChooseColorTransform(j_compress_ptr cinfo);

void ChooseColorTransform(j_decompress_ptr cinfo);

}  // namespace pdfcore

#endif  // PDFCORE_LIB_PDFCORE_COLOR_TRANSFORM_H_
