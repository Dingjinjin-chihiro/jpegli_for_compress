#!/usr/bin/env bash
# Copyright (c) the JPEG XL Project Authors.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Helper builder file to replace the /src/build.sh one in oss-fuzz/

if [[ -z "${FUZZING_ENGINE:-}" ]]; then
  echo "Don't call this script directly. Use ./ci.sh ossfuzz_* commands" \
    "instead." >&2
  exit 1
fi

set -eux

self=$(realpath "$0")
mydir=$(dirname "${self}")

main() {
  # Build the fuzzers in release mode but force the inclusion of PDFCORE_DASSERT
  # checks.
  build_args=(
    -G Ninja
    -DBUILD_TESTING=OFF
    -DBUILD_SHARED_LIBS=OFF
    -DPDFCORE_ENABLE_BENCHMARK=OFF
    -DPDFCORE_ENABLE_DEVTOOLS=ON
    -DPDFCORE_ENABLE_FUZZERS=ON
    -DPDFCORE_ENABLE_MANPAGES=OFF
    -DPDFCORE_ENABLE_SJPEG=OFF
    -DCMAKE_BUILD_TYPE=Release
  )
  export CXXFLAGS="${CXXFLAGS} -DPDFCORE_IS_DEBUG_BUILD"

  mkdir -p ${WORK}
  cd ${WORK}
  cmake \
    "${build_args[@]}" \
    -DPDFCORE_FUZZER_LINK_FLAGS="${LIB_FUZZING_ENGINE}" \
    "${SRC}/libjpegli"

  fuzzers=(
    color_encoding_fuzzer
    dpdfcore_jpegli_fuzzer
    fields_fuzzer
    icc_codec_fuzzer
    rans_fuzzer
    transforms_fuzzer
  )
  if [[ -n "${PDFCORE_EXTRA_ARGS:-}" ]]; then
    # Extra arguments passed to ci.sh ossfuzz commands are treated as ninja
    # targets. The environment variable is split into individual targets here,
    # which might break if passing paths with spaces, which is an unlikely use
    # case.
    fuzzers=(${PDFCORE_EXTRA_ARGS})
    echo "Building with targets: ${PDFCORE_EXTRA_ARGS}"
  fi
  ninja "${fuzzers[@]}"
}

# Build as the regular user if not already running as that user. This avoids
# having root files in the build directory.
if [[ -n "${PDFCORE_UID:-}" && "${PDFCORE_UID}" != $(id -u) ]]; then
  userspec="${PDFCORE_UID}:${PDFCORE_GID}"
  unset PDFCORE_UID
  unset PDFCORE_GID
  chroot --skip-chdir --userspec="${userspec}" / "${mydir}" "$@"
  exit $?
fi

main "$@"
