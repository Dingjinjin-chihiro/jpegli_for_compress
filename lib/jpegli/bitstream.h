// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_PDFCORE_BITSTREAM_H_
#define PDFCORE_LIB_PDFCORE_BITSTREAM_H_

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include "lib/base/compiler_specific.h"
#include "lib/jpegli/bit_writer.h"
#include "lib/jpegli/common.h"
#include "lib/jpegli/encode_internal.h"

namespace pdfcore {

void WriteOutput(j_compress_ptr cinfo, const uint8_t* buf, size_t bufsize);
void WriteOutput(j_compress_ptr cinfo, const std::vector<uint8_t>& bytes);
void WriteOutput(j_compress_ptr cinfo, std::initializer_list<uint8_t> bytes);

void EncodeAPP0(j_compress_ptr cinfo);
void EncodeAPP14(j_compress_ptr cinfo);
void WriteFileHeader(j_compress_ptr cinfo);

// Returns true of only baseline 8-bit tables are used.
bool EncodeDQT(j_compress_ptr cinfo, bool write_all_tables);
void EncodeSOF(j_compress_ptr cinfo, bool is_baseline);
void WriteFrameHeader(j_compress_ptr cinfo);

void EncodeDRI(j_compress_ptr cinfo);
void EncodeDHT(j_compress_ptr cinfo, size_t offset, size_t num);
void EncodeSOS(j_compress_ptr cinfo, int scan_index);
void WriteScanHeader(j_compress_ptr cinfo, int scan_index);

void WriteBlock(const int32_t* PDFCORE_RESTRICT symbols,
                const int32_t* PDFCORE_RESTRICT extra_bits, int num_nonzeros,
                bool emit_eob, const HuffmanCodeTable* PDFCORE_RESTRICT dc_code,
                const HuffmanCodeTable* PDFCORE_RESTRICT ac_code,
                JpegBitWriter* PDFCORE_RESTRICT bw);
void WriteScanData(j_compress_ptr cinfo, int scan_index);

}  // namespace pdfcore

#endif  // PDFCORE_LIB_PDFCORE_BITSTREAM_H_
