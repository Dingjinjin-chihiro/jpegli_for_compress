// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_TOOLS_SSIMULACRA2_H_
#define PDFCORE_TOOLS_SSIMULACRA2_H_

#include <vector>

#include "lib/base/status.h"
#include "lib/extras/packed_image.h"

struct MsssimScale {
  double avg_ssim[3 * 2];
  double avg_edgediff[3 * 4];
};

struct Msssim {
  std::vector<MsssimScale> scales;

  double Score() const;
};

// Computes the SSIMULACRA 2 score between reference image 'orig' and
// distorted image 'distorted'.
pdfcore::StatusOr<Msssim> ComputeSSIMULACRA2(
    const pdfcore::extras::PackedPixelFile& orig,
    const pdfcore::extras::PackedPixelFile& distorted);

#endif  // PDFCORE_TOOLS_SSIMULACRA2_H_
