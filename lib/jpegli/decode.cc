// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "lib/jpegli/decode.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <hwy/aligned_allocator.h>
#include <vector>

#include "lib/base/byte_order.h"
#include "lib/base/status.h"
#include "lib/base/types.h"
#include "lib/jpegli/color_quantize.h"
#include "lib/jpegli/common.h"
#include "lib/jpegli/common_internal.h"
#include "lib/jpegli/decode_internal.h"
#include "lib/jpegli/decode_marker.h"
#include "lib/jpegli/decode_scan.h"
#include "lib/jpegli/error.h"
#include "lib/jpegli/huffman.h"
#include "lib/jpegli/memory_manager.h"
#include "lib/jpegli/render.h"
#include "lib/jpegli/types.h"

namespace pdfcore {

void InitializeImage(j_decompress_ptr cinfo) {
  cinfo->restart_interval = 0;
  cinfo->saw_JFIF_marker = FALSE;
  cinfo->JFIF_major_version = 1;
  cinfo->JFIF_minor_version = 1;
  cinfo->density_unit = 0;
  cinfo->X_density = 1;
  cinfo->Y_density = 1;
  cinfo->saw_Adobe_marker = FALSE;
  cinfo->Adobe_transform = 0;
  cinfo->CCIR601_sampling = FALSE;  // not used
  cinfo->marker_list = nullptr;
  cinfo->comp_info = nullptr;
  cinfo->input_scan_number = 0;
  cinfo->input_iMCU_row = 0;
  cinfo->output_scan_number = 0;
  cinfo->output_iMCU_row = 0;
  cinfo->output_scanline = 0;
  cinfo->unread_marker = 0;
  cinfo->coef_bits = nullptr;
  // We set all these to zero since we don't yet support arithmetic coding.
  memset(cinfo->arith_dc_L, 0, sizeof(cinfo->arith_dc_L));
  memset(cinfo->arith_dc_U, 0, sizeof(cinfo->arith_dc_U));
  memset(cinfo->arith_ac_K, 0, sizeof(cinfo->arith_ac_K));
  // Initialize the private fields.
  jpeg_decomp_master* m = cinfo->master;
  m->input_buffer_.clear();
  m->input_buffer_pos_ = 0;
  m->codestream_bits_ahead_ = 0;
  m->is_multiscan_ = false;
  m->found_soi_ = false;
  m->found_dri_ = false;
  m->found_sof_ = false;
  m->found_sos_ = false;
  m->found_eoi_ = false;
  m->icc_index_ = 0;
  m->icc_total_ = 0;
  m->icc_profile_.clear();
  memset(m->dc_huff_lut_, 0, sizeof(m->dc_huff_lut_));
  memset(m->ac_huff_lut_, 0, sizeof(m->ac_huff_lut_));
  // Initialize the values to an invalid symbol so that we can recognize it
  // when reading the bit stream using a Huffman code with space > 0.
  for (size_t i = 0; i < kAllHuffLutSize; ++i) {
    m->dc_huff_lut_[i].bits = 0;
    m->dc_huff_lut_[i].value = 0xffff;
    m->ac_huff_lut_[i].bits = 0;
    m->ac_huff_lut_[i].value = 0xffff;
  }
  m->colormap_lut_ = nullptr;
  m->pixels_ = nullptr;
  m->scanlines_ = nullptr;
  m->regenerate_inverse_colormap_ = true;
  for (int i = 0; i < kMaxComponents; ++i) {
    m->dither_[i] = nullptr;
    m->error_row_[i] = nullptr;
  }
  m->output_passes_done_ = 0;
  m->xoffset_ = 0;
  m->dequant_ = nullptr;
}

void InitializeDecompressParams(j_decompress_ptr cinfo) {
  cinfo->jpeg_color_space = JCS_UNKNOWN;
  cinfo->out_color_space = JCS_UNKNOWN;
  cinfo->scale_num = 1;
  cinfo->scale_denom = 1;
  cinfo->output_gamma = 0.0f;
  cinfo->buffered_image = FALSE;
  cinfo->raw_data_out = FALSE;
  cinfo->dct_method = JDCT_DEFAULT;
  cinfo->do_fancy_upsampling = TRUE;
  cinfo->do_block_smoothing = TRUE;
  cinfo->quantize_colors = FALSE;
  cinfo->dither_mode = JDITHER_FS;
  cinfo->two_pass_quantize = TRUE;
  cinfo->desired_number_of_colors = 256;
  cinfo->enable_1pass_quant = FALSE;
  cinfo->enable_external_quant = FALSE;
  cinfo->enable_2pass_quant = FALSE;
  cinfo->actual_number_of_colors = 0;
  cinfo->colormap = nullptr;
}

void InitProgressMonitor(j_decompress_ptr cinfo, bool coef_only) {
  if (!cinfo->progress) return;
  jpeg_decomp_master* m = cinfo->master;
  int nc = cinfo->num_components;
  int estimated_num_scans =
      cinfo->progressive_mode ? 2 + 3 * nc : (m->is_multiscan_ ? nc : 1);
  cinfo->progress->pass_limit =
      static_cast<size_t>(cinfo->total_iMCU_rows) * estimated_num_scans;
  cinfo->progress->pass_counter = 0;
  if (coef_only) {
    cinfo->progress->total_passes = 1;
  } else {
    int input_passes = !cinfo->buffered_image && m->is_multiscan_ ? 1 : 0;
    bool two_pass_quant = FROM_PDFCORE_BOOL(cinfo->quantize_colors) &&
                          (cinfo->colormap != nullptr) &&
                          FROM_PDFCORE_BOOL(cinfo->two_pass_quantize) &&
                          FROM_PDFCORE_BOOL(cinfo->enable_2pass_quant);
    cinfo->progress->total_passes = input_passes + (two_pass_quant ? 2 : 1);
  }
  cinfo->progress->completed_passes = 0;
}

void InitProgressMonitorForOutput(j_decompress_ptr cinfo) {
  if (!cinfo->progress) return;
  jpeg_decomp_master* m = cinfo->master;
  int passes_per_output = cinfo->enable_2pass_quant ? 2 : 1;
  int output_passes_left = cinfo->buffered_image && !m->found_eoi_ ? 2 : 1;
  cinfo->progress->total_passes =
      m->output_passes_done_ + passes_per_output * output_passes_left;
  cinfo->progress->completed_passes = m->output_passes_done_;
}

void ProgressMonitorInputPass(j_decompress_ptr cinfo) {
  if (!cinfo->progress) return;
  cinfo->progress->pass_counter =
      ((cinfo->input_scan_number - 1) * cinfo->total_iMCU_rows +
       cinfo->input_iMCU_row);
  if (cinfo->progress->pass_counter > cinfo->progress->pass_limit) {
    cinfo->progress->pass_limit =
        static_cast<size_t>(cinfo->input_scan_number) * cinfo->total_iMCU_rows;
  }
  (*cinfo->progress->progress_monitor)(reinterpret_cast<j_common_ptr>(cinfo));
}

void ProgressMonitorOutputPass(j_decompress_ptr cinfo) {
  if (!cinfo->progress) return;
  jpeg_decomp_master* m = cinfo->master;
  int input_passes = !cinfo->buffered_image && m->is_multiscan_ ? 1 : 0;
  cinfo->progress->pass_counter = cinfo->output_scanline;
  cinfo->progress->pass_limit = cinfo->output_height;
  cinfo->progress->completed_passes = input_passes + m->output_passes_done_;
  (*cinfo->progress->progress_monitor)(reinterpret_cast<j_common_ptr>(cinfo));
}

void BuildHuffmanLookupTable(j_decompress_ptr cinfo, JHUFF_TBL* table,
                             HuffmanTableEntry* huff_lut) {
  uint32_t counts[kJpegHuffmanMaxBitLength + 1] = {};
  counts[0] = 0;
  int total_count = 0;
  int space = 1 << kJpegHuffmanMaxBitLength;
  int max_depth = 1;
  for (size_t i = 1; i <= kJpegHuffmanMaxBitLength; ++i) {
    int count = table->bits[i];
    if (count != 0) {
      max_depth = i;
    }
    counts[i] = count;
    total_count += count;
    space -= count * (1 << (kJpegHuffmanMaxBitLength - i));
  }
  uint32_t values[kJpegHuffmanAlphabetSize + 1] = {};
  uint8_t values_seen[256] = {0};
  for (int i = 0; i < total_count; ++i) {
    int value = table->huffval[i];
    if (values_seen[value]) {
      PDFCORE_ERROR("Duplicate Huffman code value %d", value);
    }
    values_seen[value] = 1;
    values[i] = value;
  }
  // Add an invalid symbol that will have the all 1 code.
  ++counts[max_depth];
  values[total_count] = kJpegHuffmanAlphabetSize;
  space -= (1 << (kJpegHuffmanMaxBitLength - max_depth));
  if (space < 0) {
    PDFCORE_ERROR("Invalid Huffman code lengths.");
  } else if (space > 0 && huff_lut[0].value != 0xffff) {
    // Re-initialize the values to an invalid symbol so that we can recognize
    // it when reading the bit stream using a Huffman code with space > 0.
    for (int i = 0; i < kJpegHuffmanLutSize; ++i) {
      huff_lut[i].bits = 0;
      huff_lut[i].value = 0xffff;
    }
  }
  BuildJpegHuffmanTable(&counts[0], &values[0], huff_lut);
}

void PrepareForScan(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  for (int i = 0; i < cinfo->comps_in_scan; ++i) {
    int comp_idx = cinfo->cur_comp_info[i]->component_index;
    int* prev_coef_bits = cinfo->coef_bits[comp_idx + cinfo->num_components];
    for (int k = std::min(cinfo->Ss, 1); k <= std::max(cinfo->Se, 9); k++) {
      prev_coef_bits[k] =
          (cinfo->input_scan_number > 0) ? cinfo->coef_bits[comp_idx][k] : 0;
    }
    for (int k = cinfo->Ss; k <= cinfo->Se; ++k) {
      cinfo->coef_bits[comp_idx][k] = cinfo->Al;
    }
  }
  AddStandardHuffmanTables(reinterpret_cast<j_common_ptr>(cinfo),
                           /*is_dc=*/false);
  AddStandardHuffmanTables(reinterpret_cast<j_common_ptr>(cinfo),
                           /*is_dc=*/true);
  // Check that all the Huffman tables needed for this scan are defined and
  // build derived lookup tables.
  for (int i = 0; i < cinfo->comps_in_scan; ++i) {
    if (cinfo->Ss == 0) {
      int dc_tbl_idx = cinfo->cur_comp_info[i]->dc_tbl_no;
      JHUFF_TBL* table = cinfo->dc_huff_tbl_ptrs[dc_tbl_idx];
      HuffmanTableEntry* huff_lut =
          &m->dc_huff_lut_[dc_tbl_idx * kJpegHuffmanLutSize];
      if (!table) {
        PDFCORE_ERROR("DC Huffman table %d not found", dc_tbl_idx);
      }
      BuildHuffmanLookupTable(cinfo, table, huff_lut);
    }
    if (cinfo->Se > 0) {
      int ac_tbl_idx = cinfo->cur_comp_info[i]->ac_tbl_no;
      JHUFF_TBL* table = cinfo->ac_huff_tbl_ptrs[ac_tbl_idx];
      HuffmanTableEntry* huff_lut =
          &m->ac_huff_lut_[ac_tbl_idx * kJpegHuffmanLutSize];
      if (!table) {
        PDFCORE_ERROR("AC Huffman table %d not found", ac_tbl_idx);
      }
      BuildHuffmanLookupTable(cinfo, table, huff_lut);
    }
  }
  // Copy quantization tables into comp_info.
  for (int i = 0; i < cinfo->comps_in_scan; ++i) {
    jpeg_component_info* comp = cinfo->cur_comp_info[i];
    int quant_tbl_idx = comp->quant_tbl_no;
    JQUANT_TBL* quant_table = cinfo->quant_tbl_ptrs[quant_tbl_idx];
    if (!quant_table) {
      PDFCORE_ERROR("Quantization table with index %d not found", quant_tbl_idx);
    }
    if (comp->quant_table == nullptr) {
      comp->quant_table = Allocate<JQUANT_TBL>(cinfo, 1, JPOOL_IMAGE);
      memcpy(comp->quant_table, quant_table, sizeof(JQUANT_TBL));
    }
  }
  if (cinfo->comps_in_scan == 1) {
    const auto& comp = *cinfo->cur_comp_info[0];
    cinfo->MCUs_per_row = DivCeil(cinfo->image_width * comp.h_samp_factor,
                                  cinfo->max_h_samp_factor * DCTSIZE);
    cinfo->MCU_rows_in_scan = DivCeil(cinfo->image_height * comp.v_samp_factor,
                                      cinfo->max_v_samp_factor * DCTSIZE);
    m->mcu_rows_per_iMCU_row_ = cinfo->cur_comp_info[0]->v_samp_factor;
  } else {
    cinfo->MCU_rows_in_scan = cinfo->total_iMCU_rows;
    cinfo->MCUs_per_row = m->iMCU_cols_;
    m->mcu_rows_per_iMCU_row_ = 1;
    size_t mcu_size = 0;
    for (int i = 0; i < cinfo->comps_in_scan; ++i) {
      jpeg_component_info* comp = cinfo->cur_comp_info[i];
      mcu_size +=
          static_cast<size_t>(comp->h_samp_factor) * comp->v_samp_factor;
    }
    if (mcu_size > D_MAX_BLOCKS_IN_MCU) {
      PDFCORE_ERROR("MCU size too big");
    }
  }
  memset(m->last_dc_coeff_, 0, sizeof(m->last_dc_coeff_));
  m->restarts_to_go_ = cinfo->restart_interval;
  m->next_restart_marker_ = 0;
  m->eobrun_ = -1;
  m->scan_mcu_row_ = 0;
  m->scan_mcu_col_ = 0;
  m->codestream_bits_ahead_ = 0;
  ++cinfo->input_scan_number;
  cinfo->input_iMCU_row = 0;
  PrepareForiMCURow(cinfo);
  cinfo->global_state = kDecProcessScan;
}

int ConsumeInput(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  if (cinfo->global_state == kDecProcessScan && m->streaming_mode_ &&
      cinfo->input_iMCU_row > cinfo->output_iMCU_row) {
    // Prevent input from getting ahead of output in streaming mode.
    return JPEG_SUSPENDED;
  }
  jpeg_source_mgr* src = cinfo->src;
  int status;
  for (;;) {
    const uint8_t* data;
    size_t len;
    if (m->input_buffer_.empty()) {
      data = cinfo->src->next_input_byte;
      len = cinfo->src->bytes_in_buffer;
    } else {
      data = &m->input_buffer_[m->input_buffer_pos_];
      len = m->input_buffer_.size() - m->input_buffer_pos_;
    }
    size_t pos = 0;
    if (cinfo->global_state == kDecProcessScan) {
      status = ProcessScan(cinfo, data, len, &pos, &m->codestream_bits_ahead_);
    } else {
      status = ProcessMarkers(cinfo, data, len, &pos);
    }
    if (m->input_buffer_.empty()) {
      cinfo->src->next_input_byte += pos;
      cinfo->src->bytes_in_buffer -= pos;
    } else {
      m->input_buffer_pos_ += pos;
      size_t bytes_left = m->input_buffer_.size() - m->input_buffer_pos_;
      if (bytes_left <= src->bytes_in_buffer) {
        src->next_input_byte += (src->bytes_in_buffer - bytes_left);
        src->bytes_in_buffer = bytes_left;
        m->input_buffer_.clear();
        m->input_buffer_pos_ = 0;
      }
    }
    if (status == kHandleRestart) {
      PDFCORE_DASSERT(m->input_buffer_.size() <=
                     m->input_buffer_pos_ + src->bytes_in_buffer);
      m->input_buffer_.clear();
      m->input_buffer_pos_ = 0;
      if (cinfo->unread_marker == 0xd0 + m->next_restart_marker_) {
        cinfo->unread_marker = 0;
      } else {
        if (!(*cinfo->src->resync_to_restart)(cinfo, m->next_restart_marker_)) {
          return JPEG_SUSPENDED;
        }
      }
      m->next_restart_marker_ += 1;
      m->next_restart_marker_ &= 0x7;
      m->restarts_to_go_ = cinfo->restart_interval;
      if (cinfo->unread_marker != 0) {
        PDFCORE_WARN("Failed to resync to next restart marker, skipping scan.");
        return JPEG_SCAN_COMPLETED;
      }
      continue;
    }
    if (status == kHandleMarkerProcessor) {
      PDFCORE_DASSERT(m->input_buffer_.size() <=
                     m->input_buffer_pos_ + src->bytes_in_buffer);
      m->input_buffer_.clear();
      m->input_buffer_pos_ = 0;
      if (!(*GetMarkerProcessor(cinfo))(cinfo)) {
        return JPEG_SUSPENDED;
      }
      cinfo->unread_marker = 0;
      continue;
    }
    if (status != kNeedMoreInput) {
      break;
    }
    if (m->input_buffer_.empty()) {
      PDFCORE_DASSERT(m->input_buffer_pos_ == 0);
      m->input_buffer_.assign(src->next_input_byte,
                              src->next_input_byte + src->bytes_in_buffer);
    }
    if (!(*cinfo->src->fill_input_buffer)(cinfo)) {
      m->input_buffer_.clear();
      m->input_buffer_pos_ = 0;
      return JPEG_SUSPENDED;
    }
    if (src->bytes_in_buffer == 0) {
      PDFCORE_ERROR("Empty input.");
    }
    m->input_buffer_.insert(m->input_buffer_.end(), src->next_input_byte,
                            src->next_input_byte + src->bytes_in_buffer);
  }
  if (status == JPEG_SCAN_COMPLETED) {
    cinfo->global_state = kDecProcessMarkers;
  } else if (status == JPEG_REACHED_SOS) {
    if (cinfo->global_state == kDecInHeader) {
      cinfo->global_state = kDecHeaderDone;
    } else {
      PrepareForScan(cinfo);
    }
  }
  return status;
}

bool IsInputReady(j_decompress_ptr cinfo) {
  if (cinfo->master->found_eoi_) {
    return true;
  }
  if (cinfo->input_scan_number > cinfo->output_scan_number) {
    return true;
  }
  if (cinfo->input_scan_number < cinfo->output_scan_number) {
    return false;
  }
  if (cinfo->input_iMCU_row == cinfo->total_iMCU_rows) {
    return true;
  }
  return cinfo->input_iMCU_row >
         cinfo->output_iMCU_row + (cinfo->master->streaming_mode_ ? 0 : 2);
}

bool ReadOutputPass(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  if (!m->pixels_) {
    size_t stride =
        static_cast<size_t>(cinfo->out_color_components) * cinfo->output_width;
    size_t num_samples = cinfo->output_height * stride;
    m->pixels_ = Allocate<uint8_t>(cinfo, num_samples, JPOOL_IMAGE);
    m->scanlines_ =
        Allocate<JSAMPROW>(cinfo, cinfo->output_height, JPOOL_IMAGE);
    for (size_t i = 0; i < cinfo->output_height; ++i) {
      m->scanlines_[i] = &m->pixels_[i * stride];
    }
  }
  size_t num_output_rows = 0;
  while (num_output_rows < cinfo->output_height) {
    if (IsInputReady(cinfo)) {
      ProgressMonitorOutputPass(cinfo);
      ProcessOutput(cinfo, &num_output_rows, m->scanlines_,
                    cinfo->output_height);
    } else if (ConsumeInput(cinfo) == JPEG_SUSPENDED) {
      return false;
    }
  }
  cinfo->output_scanline = 0;
  cinfo->output_iMCU_row = 0;
  return true;
}

boolean PrepareQuantizedOutput(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  if (cinfo->raw_data_out) {
    PDFCORE_ERROR("Color quantization is not supported in raw data mode.");
  }
  if (m->output_data_type_ != PDFCORE_TYPE_UINT8) {
    PDFCORE_ERROR("Color quantization must use 8-bit mode.");
  }
  if (cinfo->colormap) {
    m->quant_mode_ = 3;
  } else if (cinfo->two_pass_quantize && cinfo->enable_2pass_quant) {
    m->quant_mode_ = 2;
  } else if (cinfo->enable_1pass_quant) {
    m->quant_mode_ = 1;
  } else {
    PDFCORE_ERROR("Invalid quantization mode change");
  }
  if (m->quant_mode_ > 1 && cinfo->dither_mode == JDITHER_ORDERED) {
    cinfo->dither_mode = JDITHER_FS;
  }
  if (m->quant_mode_ == 1) {
    ChooseColorMap1Pass(cinfo);
  } else if (m->quant_mode_ == 2) {
    m->quant_pass_ = 0;
    if (!ReadOutputPass(cinfo)) {
      return FALSE;
    }
    ChooseColorMap2Pass(cinfo);
  }
  if (m->quant_mode_ == 2 ||
      (m->quant_mode_ == 3 && m->regenerate_inverse_colormap_)) {
    CreateInverseColorMap(cinfo);
  }
  if (cinfo->dither_mode == JDITHER_ORDERED) {
    CreateOrderedDitherTables(cinfo);
  } else if (cinfo->dither_mode == JDITHER_FS) {
    InitFSDitherState(cinfo);
  }
  m->quant_pass_ = 1;
  return TRUE;
}

void AllocateCoefficientBuffer(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  j_common_ptr comptr = reinterpret_cast<j_common_ptr>(cinfo);
  jvirt_barray_ptr* coef_arrays = pdfcore::Allocate<jvirt_barray_ptr>(
      cinfo, cinfo->num_components, JPOOL_IMAGE);
  for (int c = 0; c < cinfo->num_components; ++c) {
    jpeg_component_info* comp = &cinfo->comp_info[c];
    size_t height_in_blocks =
        m->streaming_mode_ ? comp->v_samp_factor : comp->height_in_blocks;
    coef_arrays[c] = (*cinfo->mem->request_virt_barray)(
        comptr, JPOOL_IMAGE, TRUE, comp->width_in_blocks, height_in_blocks,
        comp->v_samp_factor);
  }
  cinfo->master->coef_arrays = coef_arrays;
  (*cinfo->mem->realize_virt_arrays)(comptr);
}

void AllocateOutputBuffers(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  size_t iMCU_width =
      static_cast<size_t>(cinfo->max_h_samp_factor) * m->min_scaled_dct_size;
  size_t output_stride = m->iMCU_cols_ * iMCU_width;
  m->need_context_rows_ = false;
  for (int c = 0; c < cinfo->num_components; ++c) {
    if (cinfo->do_fancy_upsampling && m->v_factor[c] == 2) {
      m->need_context_rows_ = true;
    }
  }
  for (int c = 0; c < cinfo->num_components; ++c) {
    const auto& comp = cinfo->comp_info[c];
    size_t cheight =
        static_cast<size_t>(comp.v_samp_factor) * m->scaled_dct_size[c];
    int downsampled_width = output_stride / m->h_factor[c];
    m->raw_height_[c] =
        static_cast<size_t>(comp.height_in_blocks) * m->scaled_dct_size[c];
    if (m->need_context_rows_) {
      cheight *= 3;
    }
    m->raw_output_[c].Allocate(cinfo, cheight, downsampled_width);
  }
  int num_all_components =
      std::max(cinfo->out_color_components, cinfo->num_components);
  for (int c = 0; c < num_all_components; ++c) {
    m->render_output_[c].Allocate(cinfo, cinfo->max_v_samp_factor,
                                  output_stride);
  }
  m->idct_scratch_ = Allocate<float>(cinfo, 5 * DCTSIZE2, JPOOL_IMAGE_ALIGNED);
  // Padding for horizontal chroma upsampling.
  constexpr size_t kUpsamplePadding = 2 * HWY_ALIGNMENT / sizeof(float);
  m->upsample_scratch_ = Allocate<float>(
      cinfo, output_stride + kUpsamplePadding, JPOOL_IMAGE_ALIGNED);
  size_t bytes_per_sample = pdfcore_jpegli_bytes_per_sample(m->output_data_type_);
  size_t bytes_per_pixel = cinfo->out_color_components * bytes_per_sample;
  size_t scratch_stride = RoundUpTo(output_stride, HWY_ALIGNMENT);
  m->output_scratch_ = Allocate<uint8_t>(
      cinfo, bytes_per_pixel * scratch_stride, JPOOL_IMAGE_ALIGNED);
  m->smoothing_scratch_ =
      Allocate<int16_t>(cinfo, DCTSIZE2, JPOOL_IMAGE_ALIGNED);
  size_t coeffs_per_block = cinfo->num_components * DCTSIZE2;
  m->nonzeros_ = Allocate<int>(cinfo, coeffs_per_block, JPOOL_IMAGE_ALIGNED);
  m->sumabs_ = Allocate<int>(cinfo, coeffs_per_block, JPOOL_IMAGE_ALIGNED);
  m->biases_ = Allocate<float>(cinfo, coeffs_per_block, JPOOL_IMAGE_ALIGNED);
  m->dequant_ = Allocate<float>(cinfo, coeffs_per_block, JPOOL_IMAGE_ALIGNED);
  memset(m->dequant_, 0, coeffs_per_block * sizeof(float));
}

}  // namespace pdfcore

void pdfcore_jpegli_CreateDecompress(j_decompress_ptr cinfo, int version,
                             size_t structsize) {
  cinfo->mem = nullptr;
  if (structsize != sizeof(*cinfo)) {
    PDFCORE_ERROR("jpeg_decompress_struct has wrong size.");
  }
  pdfcore::InitMemoryManager(reinterpret_cast<j_common_ptr>(cinfo));
  cinfo->is_decompressor = TRUE;
  cinfo->progress = nullptr;
  cinfo->src = nullptr;
  for (auto& quant_tbl_ptr : cinfo->quant_tbl_ptrs) {
    quant_tbl_ptr = nullptr;
  }
  for (int i = 0; i < NUM_HUFF_TBLS; i++) {
    cinfo->dc_huff_tbl_ptrs[i] = nullptr;
    cinfo->ac_huff_tbl_ptrs[i] = nullptr;
  }
  cinfo->global_state = pdfcore::kDecStart;
  cinfo->sample_range_limit = nullptr;  // not used
  cinfo->rec_outbuf_height = 1;         // output works with any buffer height
  cinfo->master = new jpeg_decomp_master;
  jpeg_decomp_master* m = cinfo->master;
  for (auto& app_marker_parser : m->app_marker_parsers) {
    app_marker_parser = nullptr;
  }
  m->com_marker_parser = nullptr;
  memset(m->markers_to_save_, 0, sizeof(m->markers_to_save_));
  pdfcore::InitializeDecompressParams(cinfo);
  pdfcore::InitializeImage(cinfo);
}

void pdfcore_jpegli_destroy_decompress(j_decompress_ptr cinfo) {
  pdfcore_jpegli_destroy(reinterpret_cast<j_common_ptr>(cinfo));
}

void pdfcore_jpegli_abort_decompress(j_decompress_ptr cinfo) {
  pdfcore_jpegli_abort(reinterpret_cast<j_common_ptr>(cinfo));
}

void pdfcore_jpegli_save_markers(j_decompress_ptr cinfo, int marker_code,
                         unsigned int length_limit) {
  // TODO(szabadka) Limit our memory usage by taking into account length_limit.
  jpeg_decomp_master* m = cinfo->master;
  if (marker_code < 0xe0) {
    PDFCORE_ERROR("pdfcore_jpegli_save_markers: invalid marker code %d", marker_code);
  }
  m->markers_to_save_[marker_code - 0xe0] = 1;
}

void pdfcore_jpegli_set_marker_processor(j_decompress_ptr cinfo, int marker_code,
                                 jpeg_marker_parser_method routine) {
  jpeg_decomp_master* m = cinfo->master;
  if (marker_code == 0xfe) {
    m->com_marker_parser = routine;
  } else if (marker_code >= 0xe0 && marker_code <= 0xef) {
    m->app_marker_parsers[marker_code - 0xe0] = routine;
  } else {
    PDFCORE_ERROR("pdfcore_jpegli_set_marker_processor: invalid marker code %d",
                 marker_code);
  }
}

int pdfcore_jpegli_consume_input(j_decompress_ptr cinfo) {
  if (cinfo->global_state == pdfcore::kDecStart) {
    (*cinfo->err->reset_error_mgr)(reinterpret_cast<j_common_ptr>(cinfo));
    (*cinfo->src->init_source)(cinfo);
    pdfcore::InitializeDecompressParams(cinfo);
    pdfcore::InitializeImage(cinfo);
    cinfo->global_state = pdfcore::kDecInHeader;
  }
  if (cinfo->global_state == pdfcore::kDecHeaderDone) {
    return JPEG_REACHED_SOS;
  }
  if (cinfo->master->found_eoi_) {
    return JPEG_REACHED_EOI;
  }
  if (cinfo->global_state == pdfcore::kDecInHeader ||
      cinfo->global_state == pdfcore::kDecProcessMarkers ||
      cinfo->global_state == pdfcore::kDecProcessScan) {
    return pdfcore::ConsumeInput(cinfo);
  }
  PDFCORE_ERROR("Unexpected state %d", cinfo->global_state);
  return JPEG_REACHED_EOI;  // return value does not matter
}

int pdfcore_jpegli_read_header(j_decompress_ptr cinfo, boolean require_image) {
  if (cinfo->global_state != pdfcore::kDecStart &&
      cinfo->global_state != pdfcore::kDecInHeader) {
    PDFCORE_ERROR("pdfcore_jpegli_read_header: unexpected state %d",
                 cinfo->global_state);
  }
  if (cinfo->src == nullptr) {
    PDFCORE_ERROR("Missing source.");
  }
  for (;;) {
    int retcode = pdfcore_jpegli_consume_input(cinfo);
    if (retcode == JPEG_SUSPENDED) {
      return retcode;
    } else if (retcode == JPEG_REACHED_SOS) {
      break;
    } else if (retcode == JPEG_REACHED_EOI) {
      if (require_image) {
        PDFCORE_ERROR("pdfcore_jpegli_read_header: unexpected EOI marker.");
      }
      pdfcore_jpegli_abort_decompress(cinfo);
      return JPEG_HEADER_TABLES_ONLY;
    }
  };
  return JPEG_HEADER_OK;
}

boolean pdfcore_jpegli_read_icc_profile(j_decompress_ptr cinfo, JOCTET** icc_data_ptr,
                                unsigned int* icc_data_len) {
  if (cinfo->global_state == pdfcore::kDecStart ||
      cinfo->global_state == pdfcore::kDecInHeader) {
    PDFCORE_ERROR("pdfcore_jpegli_read_icc_profile: unexpected state %d",
                 cinfo->global_state);
  }
  if (icc_data_ptr == nullptr || icc_data_len == nullptr) {
    PDFCORE_ERROR("pdfcore_jpegli_read_icc_profile: invalid output buffer");
  }
  jpeg_decomp_master* m = cinfo->master;
  if (m->icc_profile_.empty()) {
    *icc_data_ptr = nullptr;
    *icc_data_len = 0;
    return FALSE;
  }
  *icc_data_len = m->icc_profile_.size();
  *icc_data_ptr = static_cast<JOCTET*>(malloc(*icc_data_len));
  if (*icc_data_ptr == nullptr) {
    PDFCORE_ERROR("pdfcore_jpegli_read_icc_profile: Out of memory");
  }
  memcpy(*icc_data_ptr, m->icc_profile_.data(), *icc_data_len);
  return TRUE;
}

void pdfcore_jpegli_core_output_dimensions(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  if (!m->found_sof_) {
    PDFCORE_ERROR("No SOF marker found.");
  }
  if (cinfo->raw_data_out) {
    if (cinfo->scale_num != 1 || cinfo->scale_denom != 1) {
      PDFCORE_ERROR("Output scaling is not supported in raw output mode");
    }
  }
  if (cinfo->scale_num != 1 || cinfo->scale_denom != 1) {
    int dctsize = 16;
    while (cinfo->scale_num * DCTSIZE <= cinfo->scale_denom * (dctsize - 1)) {
      --dctsize;
    }
    m->min_scaled_dct_size = dctsize;
    cinfo->output_width =
        pdfcore::DivCeil(cinfo->image_width * dctsize, DCTSIZE);
    cinfo->output_height =
        pdfcore::DivCeil(cinfo->image_height * dctsize, DCTSIZE);
    for (int c = 0; c < cinfo->num_components; ++c) {
      m->scaled_dct_size[c] = m->min_scaled_dct_size;
    }
  } else {
    cinfo->output_width = cinfo->image_width;
    cinfo->output_height = cinfo->image_height;
    m->min_scaled_dct_size = DCTSIZE;
    for (int c = 0; c < cinfo->num_components; ++c) {
      m->scaled_dct_size[c] = DCTSIZE;
    }
  }
}

void pdfcore_jpegli_calc_output_dimensions(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  pdfcore_jpegli_core_output_dimensions(cinfo);
  for (int c = 0; c < cinfo->num_components; ++c) {
    jpeg_component_info* comp = &cinfo->comp_info[c];
    m->h_factor[c] = cinfo->max_h_samp_factor / comp->h_samp_factor;
    m->v_factor[c] = cinfo->max_v_samp_factor / comp->v_samp_factor;
  }
  if (cinfo->scale_num != 1 || cinfo->scale_denom != 1) {
    for (int c = 0; c < cinfo->num_components; ++c) {
      // Prefer IDCT scaling over 2x upsampling.
      while (m->scaled_dct_size[c] < DCTSIZE && (m->v_factor[c] % 2) == 0 &&
             (m->h_factor[c] % 2) == 0) {
        m->scaled_dct_size[c] *= 2;
        m->v_factor[c] /= 2;
        m->h_factor[c] /= 2;
      }
    }
  }
  switch (cinfo->out_color_space) {
    case JCS_GRAYSCALE:
      cinfo->out_color_components = 1;
      break;
    case JCS_RGB:
    case JCS_YCbCr:
#ifdef JCS_EXTENSIONS
    case JCS_EXT_RGB:
    case JCS_EXT_BGR:
#endif
      cinfo->out_color_components = 3;
      break;
    case JCS_CMYK:
    case JCS_YCCK:
#ifdef JCS_EXTENSIONS
    case JCS_EXT_RGBX:
    case JCS_EXT_BGRX:
    case JCS_EXT_XBGR:
    case JCS_EXT_XRGB:
#endif
#ifdef JCS_ALPHA_EXTENSIONS
    case JCS_EXT_RGBA:
    case JCS_EXT_BGRA:
    case JCS_EXT_ABGR:
    case JCS_EXT_ARGB:
#endif
      cinfo->out_color_components = 4;
      break;
    default:
      cinfo->out_color_components = cinfo->num_components;
  }
  cinfo->output_components =
      cinfo->quantize_colors ? 1 : cinfo->out_color_components;
  cinfo->rec_outbuf_height = 1;
}

boolean pdfcore_jpegli_has_multiple_scans(j_decompress_ptr cinfo) {
  if (cinfo->global_state != pdfcore::kDecHeaderDone &&
      cinfo->global_state != pdfcore::kDecProcessScan &&
      cinfo->global_state != pdfcore::kDecProcessMarkers) {
    PDFCORE_ERROR("pdfcore_jpegli_has_multiple_scans: unexpected state %d",
                 cinfo->global_state);
  }
  return TO_PDFCORE_BOOL(cinfo->master->is_multiscan_);
}

boolean pdfcore_jpegli_input_complete(j_decompress_ptr cinfo) {
  return TO_PDFCORE_BOOL(cinfo->master->found_eoi_);
}

boolean pdfcore_jpegli_start_decompress(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  if (cinfo->global_state == pdfcore::kDecHeaderDone) {
    m->streaming_mode_ = !m->is_multiscan_ &&
                         !FROM_PDFCORE_BOOL(cinfo->buffered_image) &&
                         (!FROM_PDFCORE_BOOL(cinfo->quantize_colors) ||
                          !FROM_PDFCORE_BOOL(cinfo->two_pass_quantize));
    pdfcore::AllocateCoefficientBuffer(cinfo);
    pdfcore_jpegli_calc_output_dimensions(cinfo);
    pdfcore::PrepareForScan(cinfo);
    if (cinfo->quantize_colors) {
      if (cinfo->colormap != nullptr) {
        cinfo->enable_external_quant = TRUE;
      } else if (cinfo->two_pass_quantize &&
                 cinfo->out_color_space == JCS_RGB) {
        cinfo->enable_2pass_quant = TRUE;
      } else {
        cinfo->enable_1pass_quant = TRUE;
      }
    }
    pdfcore::InitProgressMonitor(cinfo, /*coef_only=*/false);
    pdfcore::AllocateOutputBuffers(cinfo);
    if (cinfo->buffered_image == TRUE) {
      cinfo->output_scan_number = 0;
      return TRUE;
    }
  } else if (!m->is_multiscan_) {
    PDFCORE_ERROR("pdfcore_jpegli_start_decompress: unexpected state %d",
                 cinfo->global_state);
  }
  if (m->is_multiscan_) {
    if (cinfo->global_state != pdfcore::kDecProcessScan &&
        cinfo->global_state != pdfcore::kDecProcessMarkers) {
      PDFCORE_ERROR("pdfcore_jpegli_start_decompress: unexpected state %d",
                   cinfo->global_state);
    }
    while (!m->found_eoi_) {
      pdfcore::ProgressMonitorInputPass(cinfo);
      if (pdfcore::ConsumeInput(cinfo) == JPEG_SUSPENDED) {
        return FALSE;
      }
    }
  }
  cinfo->output_scan_number = cinfo->input_scan_number;
  pdfcore::PrepareForOutput(cinfo);
  if (cinfo->quantize_colors) {
    return pdfcore::PrepareQuantizedOutput(cinfo);
  } else {
    return TRUE;
  }
}

boolean pdfcore_jpegli_start_output(j_decompress_ptr cinfo, int scan_number) {
  jpeg_decomp_master* m = cinfo->master;
  if (!cinfo->buffered_image) {
    PDFCORE_ERROR("pdfcore_jpegli_start_output: buffered image mode was not set");
  }
  if (cinfo->global_state != pdfcore::kDecProcessScan &&
      cinfo->global_state != pdfcore::kDecProcessMarkers) {
    PDFCORE_ERROR("pdfcore_jpegli_start_output: unexpected state %d",
                 cinfo->global_state);
  }
  cinfo->output_scan_number = std::max(1, scan_number);
  if (m->found_eoi_) {
    cinfo->output_scan_number =
        std::min(cinfo->output_scan_number, cinfo->input_scan_number);
  }
  pdfcore::InitProgressMonitorForOutput(cinfo);
  pdfcore::PrepareForOutput(cinfo);
  if (cinfo->quantize_colors) {
    return pdfcore::PrepareQuantizedOutput(cinfo);
  } else {
    return TRUE;
  }
}

boolean pdfcore_jpegli_finish_output(j_decompress_ptr cinfo) {
  if (!cinfo->buffered_image) {
    PDFCORE_ERROR("pdfcore_jpegli_finish_output: buffered image mode was not set");
  }
  if (cinfo->global_state != pdfcore::kDecProcessScan &&
      cinfo->global_state != pdfcore::kDecProcessMarkers) {
    PDFCORE_ERROR("pdfcore_jpegli_finish_output: unexpected state %d",
                 cinfo->global_state);
  }
  // Advance input to the start of the next scan, or to the end of input.
  while (cinfo->input_scan_number <= cinfo->output_scan_number &&
         !cinfo->master->found_eoi_) {
    if (pdfcore::ConsumeInput(cinfo) == JPEG_SUSPENDED) {
      return FALSE;
    }
  }
  return TRUE;
}

JDIMENSION pdfcore_jpegli_read_scanlines(j_decompress_ptr cinfo, JSAMPARRAY scanlines,
                                 JDIMENSION max_lines) {
  jpeg_decomp_master* m = cinfo->master;
  if (cinfo->global_state != pdfcore::kDecProcessScan &&
      cinfo->global_state != pdfcore::kDecProcessMarkers) {
    PDFCORE_ERROR("pdfcore_jpegli_read_scanlines: unexpected state %d",
                 cinfo->global_state);
  }
  if (cinfo->buffered_image) {
    if (cinfo->output_scan_number == 0) {
      PDFCORE_ERROR(
          "pdfcore_jpegli_read_scanlines: "
          "pdfcore_jpegli_start_output() was not called");
    }
  } else if (m->is_multiscan_ && !m->found_eoi_) {
    PDFCORE_ERROR(
        "pdfcore_jpegli_read_scanlines: "
        "pdfcore_jpegli_start_decompress() did not finish");
  }
  if (cinfo->output_scanline + max_lines > cinfo->output_height) {
    max_lines = cinfo->output_height - cinfo->output_scanline;
  }
  pdfcore::ProgressMonitorOutputPass(cinfo);
  size_t num_output_rows = 0;
  while (num_output_rows < max_lines) {
    if (pdfcore::IsInputReady(cinfo)) {
      pdfcore::ProcessOutput(cinfo, &num_output_rows, scanlines, max_lines);
    } else if (pdfcore::ConsumeInput(cinfo) == JPEG_SUSPENDED) {
      break;
    }
  }
  return num_output_rows;
}

JDIMENSION pdfcore_jpegli_skip_scanlines(j_decompress_ptr cinfo, JDIMENSION num_lines) {
  // TODO(szabadka) Skip the IDCT for skipped over blocks.
  return pdfcore_jpegli_read_scanlines(cinfo, nullptr, num_lines);
}

void pdfcore_jpegli_crop_scanline(j_decompress_ptr cinfo, JDIMENSION* xoffset,
                          JDIMENSION* width) {
  jpeg_decomp_master* m = cinfo->master;
  if ((cinfo->global_state != pdfcore::kDecProcessScan &&
       cinfo->global_state != pdfcore::kDecProcessMarkers) ||
      cinfo->output_scanline != 0) {
    PDFCORE_ERROR("pdfcore_jpegli_crop_decompress: unexpected state %d",
                 cinfo->global_state);
  }
  if (cinfo->raw_data_out) {
    PDFCORE_ERROR("Output cropping is not supported in raw data mode");
  }
  if (xoffset == nullptr || width == nullptr || *width == 0 ||
      *xoffset + *width > cinfo->output_width) {
    PDFCORE_ERROR("pdfcore_jpegli_crop_scanline: Invalid arguments");
  }
  // TODO(szabadka) Skip the IDCT for skipped over blocks.
  size_t xend = *xoffset + *width;
  size_t iMCU_width =
      static_cast<size_t>(m->min_scaled_dct_size) * cinfo->max_h_samp_factor;
  *xoffset = (*xoffset / iMCU_width) * iMCU_width;
  *width = xend - *xoffset;
  cinfo->master->xoffset_ = *xoffset;
  cinfo->output_width = *width;
}

JDIMENSION pdfcore_jpegli_read_raw_data(j_decompress_ptr cinfo, JSAMPIMAGE data,
                                JDIMENSION max_lines) {
  if ((cinfo->global_state != pdfcore::kDecProcessScan &&
       cinfo->global_state != pdfcore::kDecProcessMarkers) ||
      !cinfo->raw_data_out) {
    PDFCORE_ERROR("pdfcore_jpegli_read_raw_data: unexpected state %d",
                 cinfo->global_state);
  }
  size_t iMCU_height = cinfo->max_v_samp_factor * DCTSIZE;
  if (max_lines < iMCU_height) {
    PDFCORE_ERROR("pdfcore_jpegli_read_raw_data: output buffer too small");
  }
  pdfcore::ProgressMonitorOutputPass(cinfo);
  while (!pdfcore::IsInputReady(cinfo)) {
    if (pdfcore::ConsumeInput(cinfo) == JPEG_SUSPENDED) {
      return 0;
    }
  }
  if (cinfo->output_iMCU_row < cinfo->total_iMCU_rows) {
    pdfcore::ProcessRawOutput(cinfo, data);
    return iMCU_height;
  }
  return 0;
}

jvirt_barray_ptr* pdfcore_jpegli_read_coefficients(j_decompress_ptr cinfo) {
  jpeg_decomp_master* m = cinfo->master;
  m->streaming_mode_ = false;
  if (!cinfo->buffered_image && cinfo->global_state == pdfcore::kDecHeaderDone) {
    pdfcore::AllocateCoefficientBuffer(cinfo);
    pdfcore_jpegli_calc_output_dimensions(cinfo);
    pdfcore::InitProgressMonitor(cinfo, /*coef_only=*/true);
    pdfcore::PrepareForScan(cinfo);
  }
  if (cinfo->global_state != pdfcore::kDecProcessScan &&
      cinfo->global_state != pdfcore::kDecProcessMarkers) {
    PDFCORE_ERROR("pdfcore_jpegli_read_coefficients: unexpected state %d",
                 cinfo->global_state);
  }
  if (!cinfo->buffered_image) {
    while (!m->found_eoi_) {
      pdfcore::ProgressMonitorInputPass(cinfo);
      if (pdfcore::ConsumeInput(cinfo) == JPEG_SUSPENDED) {
        return nullptr;
      }
    }
    cinfo->output_scanline = cinfo->output_height;
  }
  return m->coef_arrays;
}

boolean pdfcore_jpegli_finish_decompress(j_decompress_ptr cinfo) {
  if (cinfo->global_state != pdfcore::kDecProcessScan &&
      cinfo->global_state != pdfcore::kDecProcessMarkers) {
    PDFCORE_ERROR("pdfcore_jpegli_finish_decompress: unexpected state %d",
                 cinfo->global_state);
  }
  if (!cinfo->buffered_image && cinfo->output_scanline < cinfo->output_height) {
    PDFCORE_ERROR("Incomplete output");
  }
  while (!cinfo->master->found_eoi_) {
    if (pdfcore::ConsumeInput(cinfo) == JPEG_SUSPENDED) {
      return FALSE;
    }
  }
  (*cinfo->src->term_source)(cinfo);
  pdfcore_jpegli_abort_decompress(cinfo);
  return TRUE;
}

boolean pdfcore_jpegli_resync_to_restart(j_decompress_ptr cinfo, int desired) {
  PDFCORE_WARN("Invalid restart marker found: 0x%02x vs 0x%02x.",
              cinfo->unread_marker, 0xd0 + desired);
  // This is a trivial implementation, we just let the decoder skip the entire
  // scan and attempt to render the partial input.
  return TRUE;
}

void pdfcore_jpegli_new_colormap(j_decompress_ptr cinfo) {
  if (cinfo->global_state != pdfcore::kDecProcessScan &&
      cinfo->global_state != pdfcore::kDecProcessMarkers) {
    PDFCORE_ERROR("pdfcore_jpegli_new_colormap: unexpected state %d",
                 cinfo->global_state);
  }
  if (!cinfo->buffered_image) {
    PDFCORE_ERROR("pdfcore_jpegli_new_colormap: not in  buffered image mode");
  }
  if (!cinfo->enable_external_quant) {
    PDFCORE_ERROR("external colormap quantizer was not enabled");
  }
  if (!cinfo->quantize_colors || cinfo->colormap == nullptr) {
    PDFCORE_ERROR("pdfcore_jpegli_new_colormap: not in external colormap mode");
  }
  cinfo->master->regenerate_inverse_colormap_ = true;
}

void pdfcore_jpegli_set_output_format(j_decompress_ptr cinfo, PdfcoreDataType data_type,
                              PdfcoreEndianness endianness) {
  switch (data_type) {
    case PDFCORE_TYPE_UINT8:
    case PDFCORE_TYPE_UINT16:
    case PDFCORE_TYPE_FLOAT:
      cinfo->master->output_data_type_ = data_type;
      break;
    default:
      PDFCORE_ERROR("Unsupported data type %d", data_type);
  }
  switch (endianness) {
    case PDFCORE_NATIVE_ENDIAN:
      cinfo->master->swap_endianness_ = false;
      break;
    case PDFCORE_LITTLE_ENDIAN:
      cinfo->master->swap_endianness_ = !IsLittleEndian();
      break;
    case PDFCORE_BIG_ENDIAN:
      cinfo->master->swap_endianness_ = IsLittleEndian();
      break;
    default:
      PDFCORE_ERROR("Unsupported endianness %d", endianness);
  }
}