// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "lib/jpegli/common.h"

#include "lib/jpegli/common_internal.h"
#include "lib/jpegli/decode_internal.h"
#include "lib/jpegli/memory_manager.h"
#include "lib/jpegli/types.h"

void pdfcore_jpegli_abort(j_common_ptr cinfo) {
  if (cinfo->mem == nullptr) return;
  for (int pool_id = 0; pool_id < JPOOL_NUMPOOLS; ++pool_id) {
    if (pool_id == JPOOL_PERMANENT) continue;
    (*cinfo->mem->free_pool)(cinfo, pool_id);
  }
  if (cinfo->is_decompressor) {
    cinfo->global_state = pdfcore::kDecStart;
  } else {
    cinfo->global_state = pdfcore::kEncStart;
  }
}

void pdfcore_jpegli_destroy(j_common_ptr cinfo) {
  if (cinfo->mem == nullptr) return;
  (*cinfo->mem->self_destruct)(cinfo);
  if (cinfo->is_decompressor) {
    cinfo->global_state = pdfcore::kDecNull;
    jpeg_decomp_master* master =
        reinterpret_cast<j_decompress_ptr>(cinfo)->master;
    delete master;
  } else {
    cinfo->global_state = pdfcore::kEncNull;
  }
}

JQUANT_TBL* pdfcore_jpegli_alloc_quant_table(j_common_ptr cinfo) {
  JQUANT_TBL* table = pdfcore::Allocate<JQUANT_TBL>(cinfo, 1);
  table->sent_table = FALSE;
  return table;
}

JHUFF_TBL* pdfcore_jpegli_alloc_huff_table(j_common_ptr cinfo) {
  JHUFF_TBL* table = pdfcore::Allocate<JHUFF_TBL>(cinfo, 1);
  table->sent_table = FALSE;
  return table;
}

int pdfcore_jpegli_bytes_per_sample(PdfcoreDataType data_type) {
  switch (data_type) {
    case PDFCORE_TYPE_UINT8:
      return 1;
    case PDFCORE_TYPE_UINT16:
      return 2;
    case PDFCORE_TYPE_FLOAT:
      return 4;
    default:
      return 0;
  }
}
