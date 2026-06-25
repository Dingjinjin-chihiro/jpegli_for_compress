/* Copyright (c) the JPEG XL Project Authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file or at
 * https://developers.google.com/open-source/licenses/bsd
 */

#ifndef PDFCORE_COLOR_ENCODING_H_
#define PDFCORE_COLOR_ENCODING_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Color space of the image data. */
typedef enum {
  /** Tristimulus RGB */
  PDFCORE_COLOR_SPACE_RGB,
  /** Luminance based, the primaries in @ref PdfcoreColorEncoding must be
   * ignored. This value implies that num_color_channels in PdfcoreBasicInfo is
   * 1, any other value implies num_color_channels is 3. */
  PDFCORE_COLOR_SPACE_GRAY,
  /** XYB (opsin) color space */
  PDFCORE_COLOR_SPACE_XYB,
  /** None of the other table entries describe the color space appropriately */
  PDFCORE_COLOR_SPACE_UNKNOWN,
} PdfcoreColorSpace;

/** Built-in white points for color encoding. When decoding, the numerical xy
 * white point value can be read from the @ref PdfcoreColorEncoding white_point
 * field regardless of the enum value. When encoding, enum values except
 * ::PDFCORE_WHITE_POINT_CUSTOM override the numerical fields. Some enum values
 * match a subset of CICP (Rec. ITU-T H.273 | ISO/IEC 23091-2:2019(E)), however
 * the white point and RGB primaries are separate enums here.
 */
typedef enum {
  /** CIE Standard Illuminant D65: 0.3127, 0.3290 */
  PDFCORE_WHITE_POINT_D65 = 1,
  /** White point must be read from the @ref PdfcoreColorEncoding white_point
   * field, or as ICC profile. This enum value is not an exact match of the
   * corresponding CICP value. */
  PDFCORE_WHITE_POINT_CUSTOM = 2,
  /** CIE Standard Illuminant E (equal-energy): 1/3, 1/3 */
  PDFCORE_WHITE_POINT_E = 10,
  /** DCI-P3 from SMPTE RP 431-2: 0.314, 0.351 */
  PDFCORE_WHITE_POINT_DCI = 11,
} PdfcoreWhitePoint;

/** Built-in primaries for color encoding. When decoding, the primaries can be
 * read from the @ref PdfcoreColorEncoding primaries_red_xy, primaries_green_xy
 * and primaries_blue_xy fields regardless of the enum value. When encoding, the
 * enum values except PDFCORE_PRIMARIES_CUSTOM override the numerical fields.
 * Some enum values match a subset of CICP (Rec. ITU-T H.273 | ISO/IEC
 * 23091-2:2019(E)), however the white point and RGB primaries are separate
 * enums here.
 */
typedef enum {
  /** The CIE xy values of the red, green and blue primaries are: 0.639998686,
     0.330010138; 0.300003784, 0.600003357; 0.150002046, 0.059997204 */
  PDFCORE_PRIMARIES_SRGB = 1,
  /** Primaries must be read from the @ref PdfcoreColorEncoding primaries_red_xy,
   * primaries_green_xy and primaries_blue_xy fields, or as ICC profile. This
   * enum value is not an exact match of the corresponding CICP value. */
  PDFCORE_PRIMARIES_CUSTOM = 2,
  /** As specified in Rec. ITU-R BT.2100-1 */
  PDFCORE_PRIMARIES_2100 = 9,
  /** As specified in SMPTE RP 431-2 */
  PDFCORE_PRIMARIES_P3 = 11,
} PdfcorePrimaries;

/** Built-in transfer functions for color encoding. Enum values match a subset
 * of CICP (Rec. ITU-T H.273 | ISO/IEC 23091-2:2019(E)) unless specified
 * otherwise. */
typedef enum {
  /** As specified in ITU-R BT.709-6 */
  PDFCORE_TRANSFER_FUNCTION_709 = 1,
  /** None of the other table entries describe the transfer function. */
  PDFCORE_TRANSFER_FUNCTION_UNKNOWN = 2,
  /** The gamma exponent is 1 */
  PDFCORE_TRANSFER_FUNCTION_LINEAR = 8,
  /** As specified in IEC 61966-2-1 sRGB */
  PDFCORE_TRANSFER_FUNCTION_SRGB = 13,
  /** As specified in SMPTE ST 2084 */
  PDFCORE_TRANSFER_FUNCTION_PQ = 16,
  /** As specified in SMPTE ST 428-1 */
  PDFCORE_TRANSFER_FUNCTION_DCI = 17,
  /** As specified in Rec. ITU-R BT.2100-1 (HLG) */
  PDFCORE_TRANSFER_FUNCTION_HLG = 18,
  /** Transfer function follows power law given by the gamma value in @ref
     PdfcoreColorEncoding. Not a CICP value. */
  PDFCORE_TRANSFER_FUNCTION_GAMMA = 65535,
} PdfcoreTransferFunction;

/** Rendering intent for color encoding, as specified in ISO 15076-1:2010 */
typedef enum {
  /** vendor-specific */
  PDFCORE_RENDERING_INTENT_PERCEPTUAL = 0,
  /** media-relative */
  PDFCORE_RENDERING_INTENT_RELATIVE,
  /** vendor-specific */
  PDFCORE_RENDERING_INTENT_SATURATION,
  /** ICC-absolute */
  PDFCORE_RENDERING_INTENT_ABSOLUTE,
} PdfcoreRenderingIntent;

/** Color encoding of the image as structured information.
 */
typedef struct {
  /** Color space of the image data.
   */
  PdfcoreColorSpace color_space;

  /** Built-in white point. If this value is ::PDFCORE_WHITE_POINT_CUSTOM, must
   * use the numerical white point values from white_point_xy.
   */
  PdfcoreWhitePoint white_point;

  /** Numerical whitepoint values in CIE xy space. */
  double white_point_xy[2];

  /** Built-in RGB primaries. If this value is PDFCORE_PRIMARIES_CUSTOM, must
   * use the numerical primaries values below. This field and the custom values
   * below are unused and must be ignored if the color space is
   * PDFCORE_COLOR_SPACE_GRAY or PDFCORE_COLOR_SPACE_XYB.
   */
  PdfcorePrimaries primaries;

  /** Numerical red primary values in CIE xy space. */
  double primaries_red_xy[2];

  /** Numerical green primary values in CIE xy space. */
  double primaries_green_xy[2];

  /** Numerical blue primary values in CIE xy space. */
  double primaries_blue_xy[2];

  /** Transfer function if have_gamma is 0 */
  PdfcoreTransferFunction transfer_function;

  /** Gamma value used when transfer_function is PDFCORE_TRANSFER_FUNCTION_GAMMA
   */
  double gamma;

  /** Rendering intent defined for the color profile. */
  PdfcoreRenderingIntent rendering_intent;
} PdfcoreColorEncoding;

#ifdef __cplusplus
}
#endif

#endif /* PDFCORE_COLOR_ENCODING_H_ */
