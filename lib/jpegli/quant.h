// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_PDFCORE_QUANT_H_
#define PDFCORE_LIB_PDFCORE_QUANT_H_

#include "lib/jpegli/common.h"

namespace pdfcore {

void SetQuantMatrices(j_compress_ptr cinfo, float distances[NUM_QUANT_TBLS],
                      bool add_two_chroma_tables);

enum QuantPass {
  NO_SEARCH,
  SEARCH_FIRST_PASS,
  SEARCH_SECOND_PASS,
};

void InitQuantizer(j_compress_ptr cinfo, QuantPass pass);

}  // namespace pdfcore

#endif  // PDFCORE_LIB_PDFCORE_QUANT_H_
