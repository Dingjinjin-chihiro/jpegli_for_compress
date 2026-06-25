# Copyright (c) the JPEG XL Project Authors.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

include(jpegli_lists.cmake)

if (BUILD_TESTING OR PDFCORE_ENABLE_TOOLS)
# Library with test-only code shared between all tests / fuzzers.
add_library(pdfcore_jpegli_testlib-internal STATIC ${PDFCORE_INTERNAL_TESTLIB_FILES})
target_compile_options(pdfcore_jpegli_testlib-internal PRIVATE
  ${PDFCORE_INTERNAL_FLAGS}
  ${PDFCORE_COVERAGE_FLAGS}
)
target_compile_definitions(pdfcore_jpegli_testlib-internal PUBLIC
  -DTEST_DATA_PATH="${PDFCORE_TEST_DATA_PATH}")
target_include_directories(pdfcore_jpegli_testlib-internal PUBLIC
  "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(pdfcore_jpegli_testlib-internal
  hwy
  pdfcore_jpegli_cms
  pdfcore_jpegli_threads
)
endif()

if (NOT BUILD_TESTING)
  return()
endif()

if (NOT PNG_FOUND)
  message(FATAL_ERROR "PNG library is required by some tests")
endif()

list(APPEND PDFCORE_INTERNAL_TESTS
  # TODO(deymo): Move this to tools/
  ../tools/gauss_blur_test.cc
)

set(PDFCORE_WASM_TEST_LINK_FLAGS "")
if (EMSCRIPTEN)
  # The emscripten linking step takes too much memory and crashes during the
  # wasm-opt step when using -O2 optimization level
  set(PDFCORE_WASM_TEST_LINK_FLAGS "\
    -O1 \
    -s USE_LIBPNG=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s SINGLE_FILE=1 \
    -s EXIT_RUNTIME=1 \
    -s NODERAWFS=1 \
  ")
  if (PDFCORE_ENABLE_WASM_THREADS)
    set(PDFCORE_WASM_TEST_LINK_FLAGS "${PDFCORE_WASM_TEST_LINK_FLAGS} \
      -s PROXY_TO_PTHREAD \
      -s USE_PTHREADS=1 \
    ")
  endif()
endif()  # EMSCRIPTEN

# Individual test binaries:
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests)
foreach (TESTFILE IN LISTS PDFCORE_INTERNAL_TESTS)
  # The TESTNAME is the name without the extension or directory.
  get_filename_component(TESTNAME ${TESTFILE} NAME_WE)
  add_executable(${TESTNAME} ${TESTFILE})
  if(EMSCRIPTEN)
    set_target_properties(${TESTNAME} PROPERTIES LINK_FLAGS "${PDFCORE_WASM_TEST_LINK_FLAGS}")
  else()
    set_target_properties(${TESTNAME} PROPERTIES LINK_FLAGS "${PDFCORE_COVERAGE_LINK_FLAGS}")
  endif()
  target_compile_options(${TESTNAME} PRIVATE
    ${PDFCORE_INTERNAL_FLAGS}
    # Add coverage flags to the test binary so code in the private headers of
    # the library is also instrumented when running tests that execute it.
    ${PDFCORE_COVERAGE_FLAGS}
  )
  target_link_libraries(${TESTNAME}
    gtest
    gtest_main
    pdfcore_jpegli_testlib-internal
    pdfcore_jpegli_extras-internal
  )
  if(TESTFILE STREQUAL ../tools/gauss_blur_test.cc)
    target_link_libraries(${TESTNAME} pdfcore_jpegli_gauss_blur)
  endif()

  # Output test targets in the test directory.
  set_target_properties(${TESTNAME} PROPERTIES PREFIX "tests/")
  if (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set_target_properties(${TESTNAME} PROPERTIES COMPILE_FLAGS "-Wno-error")
  endif ()
  # 240 seconds because some build types (e.g. coverage) can be quite slow.
  gtest_discover_tests(${TESTNAME} DISCOVERY_TIMEOUT 240)
endforeach ()
