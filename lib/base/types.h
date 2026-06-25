/* Copyright (c) the JPEG XL Project Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

/** @addtogroup libpdfcore_jpegli_common
 * @{
 * @file types.h
 * @brief Data types for the JPEGLI API, for both encoding and decoding.
 */

#ifndef PDFCORE_TYPES_H_
#define PDFCORE_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#include "lib/jpegli/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A portable @c bool replacement.
 *
 * ::PDFCORE_BOOL is a "documentation" type: actually it is @c int, but in API it
 * denotes a type, whose only values are ::PDFCORE_TRUE and ::PDFCORE_FALSE.
 */
#define PDFCORE_BOOL int
/** Portable @c true replacement. */
#define PDFCORE_TRUE 1
/** Portable @c false replacement. */
#define PDFCORE_FALSE 0
/** Converts of bool-like value to either ::PDFCORE_TRUE or ::PDFCORE_FALSE. */
#define TO_PDFCORE_BOOL(C) (!!(C) ? PDFCORE_TRUE : PDFCORE_FALSE)
/** Converts PDFCORE_BOOL to C++ bool. */
#define FROM_PDFCORE_BOOL(C) (static_cast<bool>(C))

/** Data type for the sample values per channel per pixel for the output buffer
 * for pixels. This is not necessarily the same as the data type encoded in the
 * codestream. The channels are interleaved per pixel. The pixels are
 * organized row by row, left to right, top to bottom.
 * TODO(lode): support different channel orders if needed (RGB, BGR, ...)
 */
typedef struct {
  /** Amount of channels available in a pixel buffer.
   * 1: single-channel data, e.g. grayscale or a single extra channel
   * 2: single-channel + alpha
   * 3: trichromatic, e.g. RGB
   * 4: trichromatic + alpha
   * TODO(lode): this needs finetuning. It is not yet defined how the user
   * chooses output color space. CMYK+alpha needs 5 channels.
   */
  uint32_t num_channels;

  /** Data type of each channel.
   */
  PdfcoreDataType data_type;

  /** Whether multi-byte data types are represented in big endian or little
   * endian format. This applies to ::PDFCORE_TYPE_UINT16 and
   * ::PDFCORE_TYPE_FLOAT.
   */
  PdfcoreEndianness endianness;

  /** Align scanlines to a multiple of align bytes, or 0 to require no
   * alignment at all (which has the same effect as value 1)
   */
  size_t align;
} PdfcorePixelFormat;

/** Settings for the interpretation of UINT input and output buffers.
 *  (buffers using a FLOAT data type are not affected by this)
 */
typedef enum {
  /** This is the default setting, where the encoder expects the input pixels
   * to use the full range of the pixel format data type (e.g. for UINT16, the
   * input range is 0 .. 65535 and the value 65535 is mapped to 1.0 when
   * converting to float), and the decoder uses the full range to output
   * pixels. If the bit depth in the basic info is different from this, the
   * encoder expects the values to be rescaled accordingly (e.g. multiplied by
   * 65535/4095 for a 12-bit image using UINT16 input data type). */
  PDFCORE_BIT_DEPTH_FROM_PIXEL_FORMAT = 0,

  /** If this setting is selected, the encoder expects the input pixels to be
   * in the range defined by the bits_per_sample value of the basic info (e.g.
   * for 12-bit images using UINT16 input data types, the allowed range is
   * 0 .. 4095 and the value 4095 is mapped to 1.0 when converting to float),
   * and the decoder outputs pixels in this range. */
  PDFCORE_BIT_DEPTH_FROM_CODESTREAM = 1,

  /** This setting can only be used in the decoder to select a custom range for
   * pixel output */
  PDFCORE_BIT_DEPTH_CUSTOM = 2,
} PdfcoreBitDepthType;

/** Data type for describing the interpretation of the input and output buffers
 * in terms of the range of allowed input and output pixel values. */
typedef struct {
  /** Bit depth setting, see comment on @ref PdfcoreBitDepthType */
  PdfcoreBitDepthType type;

  /** Custom bits per sample */
  uint32_t bits_per_sample;

  /** Custom exponent bits per sample */
  uint32_t exponent_bits_per_sample;
} PdfcoreBitDepth;

/** Data type holding the 4-character type name of an ISOBMFF box.
 */
typedef char PdfcoreBoxType[4];

#ifdef __cplusplus
}
#endif

#endif /* PDFCORE_TYPES_H_ */

/** @}*/
