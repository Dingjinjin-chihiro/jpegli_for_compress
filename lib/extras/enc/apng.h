// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_EXTRAS_ENC_APNG_H_
#define PDFCORE_LIB_EXTRAS_ENC_APNG_H_

// Encodes APNG images in memory.

#include <memory>

#include "lib/extras/enc/encode.h"

namespace pdfcore {
namespace extras {

std::unique_ptr<Encoder> GetAPNGEncoder();

}  // namespace extras
}  // namespace pdfcore

#endif  // PDFCORE_LIB_EXTRAS_ENC_APNG_H_
