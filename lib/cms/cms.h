// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_CMS_H_
#define PDFCORE_CMS_H_

// ICC profiles and color space conversions.

#include <jpegli/jpegli_cms_export.h>

#include "lib/cms/cms_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

PDFCORE_CMS_EXPORT const PdfcoreCmsInterface* PdfcoreGetDefaultCms();

#ifdef __cplusplus
}
#endif

#endif  // PDFCORE_CMS_H_
