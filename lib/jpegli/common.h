// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This file contains the C API of the common encoder/decoder part of libjpegli
// library, which is based on the C API of libjpeg, with the function names
// changed from jpeg_* to pdfcore_jpegli_*, while compressor and decompressor object
// definitions are included directly from jpeglib.h
//
// Applications can use the libjpegli library in one of the following ways:
//
//  (1) Include jpegli/encode.h and/or jpegli/decode.h, update the function
//      names of the API and link against libjpegli.
//
//  (2) Leave the application code unchanged, but replace the libjpeg.so library
//      with the one built by this project that is API- and ABI-compatible with
//      libjpeg-turbo's version of libjpeg.so.

#ifndef PDFCORE_LIB_PDFCORE_COMMON_H_
#define PDFCORE_LIB_PDFCORE_COMMON_H_

#include "lib/base/include_jpeglib.h"  // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

struct jpeg_error_mgr* pdfcore_jpegli_std_error(struct jpeg_error_mgr* err);

void pdfcore_jpegli_abort(j_common_ptr cinfo);

void pdfcore_jpegli_destroy(j_common_ptr cinfo);

JQUANT_TBL* pdfcore_jpegli_alloc_quant_table(j_common_ptr cinfo);

JHUFF_TBL* pdfcore_jpegli_alloc_huff_table(j_common_ptr cinfo);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // PDFCORE_LIB_PDFCORE_COMMON_H_
