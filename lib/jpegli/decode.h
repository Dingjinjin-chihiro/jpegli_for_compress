// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This file contains the C API of the decoder part of the libjpegli library,
// which is based on the C API of libjpeg, with the function names changed from
// jpeg_* to pdfcore_jpegli_*, while decompressor object definitions are included
// directly from jpeglib.h
//
// Applications can use the libjpegli library in one of the following ways:
//
//  (1) Include jpegli/encode.h and/or jpegli/decode.h, update the function
//      names of the API and link against libjpegli.
//
//  (2) Leave the application code unchanged, but replace the libjpeg.so library
//      with the one built by this project that is API- and ABI-compatible with
//      libjpeg-turbo's version of libjpeg.so.

#ifndef PDFCORE_LIB_PDFCORE_DECODE_H_
#define PDFCORE_LIB_PDFCORE_DECODE_H_

#include <cstddef>
#include <cstdio>

#include "lib/jpegli/common.h"
#include "lib/jpegli/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define pdfcore_jpegli_create_decompress(cinfo)              \
  pdfcore_jpegli_CreateDecompress((cinfo), JPEG_LIB_VERSION, \
                          (size_t)sizeof(struct jpeg_decompress_struct))

void pdfcore_jpegli_CreateDecompress(j_decompress_ptr cinfo, int version,
                             size_t structsize);

void pdfcore_jpegli_stdio_src(j_decompress_ptr cinfo, FILE *infile);

void pdfcore_jpegli_mem_src(j_decompress_ptr cinfo, const unsigned char *inbuffer,
                    unsigned long insize /* NOLINT */);

int pdfcore_jpegli_read_header(j_decompress_ptr cinfo, boolean require_image);

boolean pdfcore_jpegli_start_decompress(j_decompress_ptr cinfo);

JDIMENSION pdfcore_jpegli_read_scanlines(j_decompress_ptr cinfo, JSAMPARRAY scanlines,
                                 JDIMENSION max_lines);

JDIMENSION pdfcore_jpegli_skip_scanlines(j_decompress_ptr cinfo, JDIMENSION num_lines);

void pdfcore_jpegli_crop_scanline(j_decompress_ptr cinfo, JDIMENSION *xoffset,
                          JDIMENSION *width);

boolean pdfcore_jpegli_finish_decompress(j_decompress_ptr cinfo);

JDIMENSION pdfcore_jpegli_read_raw_data(j_decompress_ptr cinfo, JSAMPIMAGE data,
                                JDIMENSION max_lines);

jvirt_barray_ptr *pdfcore_jpegli_read_coefficients(j_decompress_ptr cinfo);

boolean pdfcore_jpegli_has_multiple_scans(j_decompress_ptr cinfo);

boolean pdfcore_jpegli_start_output(j_decompress_ptr cinfo, int scan_number);

boolean pdfcore_jpegli_finish_output(j_decompress_ptr cinfo);

boolean pdfcore_jpegli_input_complete(j_decompress_ptr cinfo);

int pdfcore_jpegli_consume_input(j_decompress_ptr cinfo);

#if JPEG_LIB_VERSION >= 80
void pdfcore_jpegli_core_output_dimensions(j_decompress_ptr cinfo);
#endif
void pdfcore_jpegli_calc_output_dimensions(j_decompress_ptr cinfo);

void pdfcore_jpegli_save_markers(j_decompress_ptr cinfo, int marker_code,
                         unsigned int length_limit);

void pdfcore_jpegli_set_marker_processor(j_decompress_ptr cinfo, int marker_code,
                                 jpeg_marker_parser_method routine);

boolean pdfcore_jpegli_resync_to_restart(j_decompress_ptr cinfo, int desired);

boolean pdfcore_jpegli_read_icc_profile(j_decompress_ptr cinfo, JOCTET **icc_data_ptr,
                                unsigned int *icc_data_len);

void pdfcore_jpegli_abort_decompress(j_decompress_ptr cinfo);

void pdfcore_jpegli_destroy_decompress(j_decompress_ptr cinfo);

void pdfcore_jpegli_new_colormap(j_decompress_ptr cinfo);

//
// New API functions that are not available in libjpeg
//
// NOTE: This part of the API is still experimental and will probably change in
// the future.
//

void pdfcore_jpegli_set_output_format(j_decompress_ptr cinfo, PdfcoreDataType data_type,
                              PdfcoreEndianness endianness);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // PDFCORE_LIB_PDFCORE_DECODE_H_
