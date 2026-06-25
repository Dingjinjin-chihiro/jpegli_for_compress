// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_EXTRAS_COLOR_DESCRIPTION_H_
#define PDFCORE_LIB_EXTRAS_COLOR_DESCRIPTION_H_

#include <string>

#include "lib/base/compiler_specific.h"
#include "lib/base/status.h"
#include "lib/cms/color_encoding.h"

namespace pdfcore {

// Parse the color description into a PdfcoreColorEncoding "RGB_D65_SRG_Rel_Lin".
Status ParseDescription(const std::string& description,
                        PdfcoreColorEncoding* PDFCORE_RESTRICT c);

}  // namespace pdfcore

#endif  // PDFCORE_LIB_EXTRAS_COLOR_DESCRIPTION_H_
