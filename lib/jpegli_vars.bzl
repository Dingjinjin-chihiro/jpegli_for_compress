# Copyright (c) the JPEG XL Project Authors. All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Extra build variables.

pdfcore_jpegli_root_package = "jpegli"

libpdfcore_jpegli_deps_exr = ["@openexr//:OpenEXR"]
libpdfcore_jpegli_deps_gif = ["@giflib//:giflib"]
libpdfcore_jpegli_deps_gtest = ["@googletest//:gtest_main"]
libpdfcore_jpegli_deps_hwy = ["@highway//:hwy"]
libpdfcore_jpegli_deps_hwy_nanobenchmark = ["@highway//:nanobenchmark"]
libpdfcore_jpegli_deps_hwy_test_util = ["@highway//:hwy_test_util"]
libpdfcore_jpegli_deps_jpeg = ["@libjpeg_turbo//:jpeg"]
libpdfcore_jpegli_deps_png = ["@libpng//:png"]
libpdfcore_jpegli_deps_runfiles = ["@bazel_tools//tools/cpp/runfiles"]
libpdfcore_jpegli_deps_skcms = ["@skcms//:skcms"]
libpdfcore_jpegli_deps_testdata = ["//:testdata"]
libpdfcore_jpegli_deps_webp = ["@libwebp//:webp"]

libpdfcore_jpegli_test_shards = {
    "jpegli/decode_api_test": 10,
    "jpegli/encode_api_test": 4,
    "jpegli/input_suspension_test": 6,
    "jpegli/output_suspension_test": 2,
    "jpegli/ans_test": 2,
    "jpegli/linalg_test": 2,
    "jpegli/modular_test": 4,
    "jpegli/roundtrip_test": 4,
    "jpegli/xorshift128plus_test": 2,
    "jpegli/ac_strategy_test": 10,  # TODO(eustas): separate heavy shard
    "jpegli/dct_test": 32,
    "jpegli/decode_test": 10,  # TODO(eustas): separate heavy shard
    "jpegli/fast_dct_test": 8,  # TODO(eustas): separate ultra-heavy shard
    "jpegli/fast_math_test": 10,  # TODO(eustas): separate heavy shard
    "jpegli/jpegli_test": 10,  # TODO(eustas): separate heavy shard
    "jpegli/render_pipeline/render_pipeline_test": 10,
}

libpdfcore_jpegli_test_timeouts = {
    "jpegli/fast_dct_test": "long",
    "jpegli/dct_test": "long",
}
