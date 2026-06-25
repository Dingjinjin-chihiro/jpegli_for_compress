# Copyright (c) the JPEG XL Project Authors.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Tools are added conditionally below.
set(TOOL_BINARIES)
# Tools that depend on jpegli internal functions.
set(INTERNAL_TOOL_BINARIES)
set(FUZZER_CORPUS_BINARIES)

add_library(pdfcore_jpegli_tool STATIC EXCLUDE_FROM_ALL
  cmdline.cc
  no_memory_manager.cc
  speed_stats.cc
  tool_version.cc
  tracking_memory_manager.cc
)
# PDFCORE_INTERNAL_FLAGS is set inside lib/
# target_compile_options(pdfcore_jpegli_tool PUBLIC "${PDFCORE_INTERNAL_FLAGS}")
target_compile_options(pdfcore_jpegli_tool PRIVATE -DPDFCORE_COMPILER_ID="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
target_include_directories(pdfcore_jpegli_tool PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(pdfcore_jpegli_tool PUBLIC pdfcore_jpegli_base hwy)

# The PDFCORE_VERSION is set from the builders.
if(NOT DEFINED PDFCORE_VERSION OR PDFCORE_VERSION STREQUAL "")
  find_package(Git QUIET)
  execute_process(
      COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
      OUTPUT_VARIABLE GIT_REV
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      ERROR_QUIET)
  string(STRIP "${GIT_REV}" GIT_REV)
  if(GIT_REV STREQUAL "")
    set(PDFCORE_VERSION "(unknown)")
  endif()
endif()

if(NOT DEFINED PDFCORE_VERSION OR PDFCORE_VERSION STREQUAL "")
  # We are building from a git environment and the user didn't set
  # PDFCORE_VERSION. Make a target that computes the GIT_REV at build-time always
  # but only updates the file if it changed. This allows rebuilds without
  # modifying cmake files to update the PDFCORE_VERSION.
  message(STATUS "Building with PDFCORE_VERSION=${GIT_REV} (auto-updated)")
  add_custom_target(
    tool_version_git
    ${CMAKE_COMMAND}
      -D PDFCORE_ROOT_DIR=${CMAKE_SOURCE_DIR}
      -D DST=${CMAKE_CURRENT_BINARY_DIR}/tool_version_git.h
      -P ${CMAKE_CURRENT_SOURCE_DIR}/git_version.cmake
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/tool_version_git.h"
  )
  add_dependencies(pdfcore_jpegli_tool tool_version_git)

  set_source_files_properties(tool_version.cc PROPERTIES
    COMPILE_DEFINITIONS PDFCORE_VERSION_FROM_GIT=1)
  target_include_directories(pdfcore_jpegli_tool PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
  # Note: Ninja looks for dependencies on the pdfcore_jpegli_tool target before running
  # the tool_version_git targets, so when updating the tool_version_git.h the
  # pdfcore_jpegli_tool target is not rebuilt. This forces to generate it at configure time
  # if needed.
  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -D PDFCORE_ROOT_DIR=${CMAKE_SOURCE_DIR}
      -D DST=${CMAKE_CURRENT_BINARY_DIR}/tool_version_git.h
      -P ${CMAKE_CURRENT_SOURCE_DIR}/git_version.cmake)
else()
  message(STATUS "Building with PDFCORE_VERSION=${PDFCORE_VERSION}")
  set_source_files_properties(tool_version.cc PROPERTIES
    COMPILE_DEFINITIONS PDFCORE_VERSION=\"${PDFCORE_VERSION}\")
endif()

add_library(pdfcore_jpegli_gauss_blur STATIC #EXCLUDE_FROM_ALL
  gauss_blur.cc
)
target_compile_options(pdfcore_jpegli_gauss_blur PUBLIC "${PDFCORE_INTERNAL_FLAGS}")
target_include_directories(pdfcore_jpegli_gauss_blur PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(pdfcore_jpegli_gauss_blur PUBLIC hwy)

if(PDFCORE_ENABLE_TOOLS)
  # Depends on parts of pdfcore_jpegli_extras that are only built if libjpeg is found and
  # jpegli is enabled.
  add_executable(cjpegli cjpegli.cc)
  target_link_libraries(cjpegli jpegli-static)
  add_executable(djpegli djpegli.cc)
  target_link_libraries(djpegli jpegli-static)
  list(APPEND INTERNAL_TOOL_BINARIES cjpegli djpegli)
endif()  # PDFCORE_ENABLE_TOOLS

# Other developer tools.
if(PDFCORE_ENABLE_DEVTOOLS)
  list(APPEND INTERNAL_TOOL_BINARIES
    ssimulacra2
  )

  add_executable(ssimulacra2 ssimulacra2_main.cc ssimulacra2.cc)
  target_link_libraries(ssimulacra2 pdfcore_jpegli_gauss_blur)

  list(APPEND FUZZER_CORPUS_BINARIES pdfcore_jpegli_dec_fuzzer_corpus)
  add_executable(pdfcore_jpegli_dec_fuzzer_corpus pdfcore_jpegli_dec_fuzzer_corpus.cc)
  target_link_libraries(pdfcore_jpegli_dec_fuzzer_corpus
    jpegli-static
    pdfcore_jpegli_tool
    pdfcore_jpegli_threads
  )
endif()  # PDFCORE_ENABLE_DEVTOOLS

# Benchmark tools.
if(PDFCORE_ENABLE_BENCHMARK AND PDFCORE_ENABLE_TOOLS)
  list(APPEND INTERNAL_TOOL_BINARIES
    benchmark_xl
  )

  add_executable(benchmark_xl
    benchmark/benchmark_xl.cc
    benchmark/benchmark_args.cc
    benchmark/benchmark_codec.cc
    benchmark/benchmark_file_io.cc
    benchmark/benchmark_stats.cc
    benchmark/benchmark_utils.cc
    benchmark/benchmark_utils.h
    benchmark/benchmark_codec_jpeg.cc
    benchmark/benchmark_codec_jpeg.h
    ssimulacra2.cc
    ../third_party/dirent.cc
  )
  target_link_libraries(benchmark_xl Threads::Threads)
  target_link_libraries(benchmark_xl pdfcore_jpegli_gauss_blur) # for ssimulacra

if(MINGW)
  # MINGW doesn't support glob.h.
  target_compile_definitions(benchmark_xl PRIVATE "-DHAS_GLOB=0")
  endif() # MINGW
endif()  # PDFCORE_ENABLE_BENCHMARK

# All tool binaries depend on "jpegli" library and the tool helpers.
foreach(BINARY IN LISTS INTERNAL_TOOL_BINARIES)
  target_link_libraries("${BINARY}"
    pdfcore_jpegli_extras-internal
    pdfcore_jpegli_threads
    pdfcore_jpegli_tool
    pdfcore_jpegli_cms
  )
endforeach()

list(APPEND TOOL_BINARIES ${INTERNAL_TOOL_BINARIES} ${FUZZER_CORPUS_BINARIES})

foreach(BINARY IN LISTS TOOL_BINARIES)
  if(EMSCRIPTEN)
    set(PDFCORE_WASM_TOOLS_LINK_FLAGS "\
      -s USE_LIBPNG=1 \
      -s ALLOW_MEMORY_GROWTH=1 \
    ")
    if (PDFCORE_ENABLE_WASM_THREADS)
      set(PDFCORE_WASM_TOOLS_LINK_FLAGS "${PDFCORE_WASM_TOOLS_LINK_FLAGS} \
        -s USE_PTHREADS=1 \
        -s PTHREAD_POOL_SIZE=16 \
      ")
    endif()
    set_target_properties(${BINARY} PROPERTIES LINK_FLAGS "${PDFCORE_WASM_TOOLS_LINK_FLAGS}")
  endif()

  # Tools are CLI.
  set_target_properties(${BINARY} PROPERTIES MACOSX_BUNDLE OFF)

  # Attach manifest that tells Windows to use UTF-8 for eg. fopen
  if(WIN32)
    # Accommodate cl, clang-cl and clang with GNU-like command
    if(NOT MINGW)
      target_sources(${BINARY} PRIVATE utf8.manifest)
    # Since CMake in MINGW doesn't support linking
    # .manifest file, do it with .rc file
    elseif(MINGW)
      target_sources(${BINARY} PRIVATE utf8.rc)
    endif()
  endif()
endforeach()

install(TARGETS ${TOOL_BINARIES} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
message(STATUS "Building tools: ${TOOL_BINARIES}")

# pdfcore_jpegli_dec_fuzzer builds even when not PDFCORE_ENABLE_TOOLS
set(FUZZER_BINARIES pdfcore_jpegli_dec_fuzzer)

# Fuzzers.
foreach(FUZZER IN LISTS FUZZER_BINARIES)
  if(PDFCORE_ENABLE_FUZZERS)
    set(BINARY "${FUZZER}")
    add_executable("${BINARY}" "${BINARY}.cc")
    target_link_libraries("${BINARY}" ${PDFCORE_FUZZER_LINK_FLAGS})
  else()
    # When not enabled we want a lightweight alternative for regular fuzzers
    # that just run the target.
    set(BINARY "${FUZZER}_runner")
    add_executable("${BINARY}" EXCLUDE_FROM_ALL
        "fuzzer_stub.cc" "${FUZZER}.cc")
  endif()  # PDFCORE_ENABLE_FUZZERS
  target_include_directories("${BINARY}" PRIVATE "${CMAKE_SOURCE_DIR}")
  target_link_libraries("${BINARY}" jpegli-static)
endforeach()

if(PDFCORE_ENABLE_JNI)
find_package(JNI QUIET)
find_package(Java QUIET)

if (JNI_FOUND AND Java_FOUND)
  include(UseJava)
  # NB: *_jni_onload.cc might be necessary for Android; not used yet.

  # jpegli wrapper

  add_library(pdfcore_jpegli_jni SHARED jni/org/jpeg/jpegli/wrapper/encoder_jni.cc)
  target_include_directories(pdfcore_jpegli_jni PRIVATE "${JNI_INCLUDE_DIRS}" "${PROJECT_SOURCE_DIR}")
  target_include_directories(pdfcore_jpegli_jni PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include/jpegli"
  )
  target_link_libraries(pdfcore_jpegli_jni PUBLIC jpegli-static)

  add_jar(pdfcore_jpegli_jni_wrapper SOURCES
    jni/org/jpeg/jpegli/wrapper/Encoder.java
    jni/org/jpeg/jpegli/wrapper/JniHelper.java
    OUTPUT_NAME org.jpeg.jpegli
  )
  get_target_property(PDFCORE_JNI_WRAPPER_JAR pdfcore_jpegli_jni_wrapper JAR_FILE)

  add_jar(pdfcore_jpegli_jni_wrapper_test
    SOURCES jni/org/jpeg/jpegli/wrapper/EncoderTest.java
    INCLUDE_JARS pdfcore_jpegli_jni_wrapper
  )
  get_target_property(PDFCORE_JNI_WRAPPER_TEST_JAR pdfcore_jpegli_jni_wrapper_test JAR_FILE)

  if(NOT SANITIZER MATCHES ".san")
    # NB: Vanilla OpenJDK 8 / 11 are known to work well (i.e. either
    #     "which java" or JAVA_HOME environment variable point to the path like
    #     "/usr/lib/jvm/java-xx-openjdk-yyy" on Debian Linux).
    add_test(
      NAME test_pdfcore_jpegli_jni_wrapper
      COMMAND ${Java_JAVA_EXECUTABLE}
              -cp "${PDFCORE_JNI_WRAPPER_JAR}:${PDFCORE_JNI_WRAPPER_TEST_JAR}"
              -Dorg.jpeg.jpegli.wrapper.lib=$<TARGET_FILE:pdfcore_jpegli_jni>
              org.jpeg.jpegli.wrapper.EncoderTest
    )
  endif()  # PDFCORE_ENABLE_FUZZERS
endif()  # JNI_FOUND & Java_FOUND
endif()  # PDFCORE_ENABLE_JNI

# End-to-end tests for the tools
if(PDFCORE_TEST_TOOLS)
find_program (BASH_PROGRAM bash)
if (BASH_PROGRAM)
  set(TEST_SCRIPTS)
  find_package(JPEG)
  if (JPEG_FOUND)
    list(APPEND TEST_SCRIPTS pdfcore_jpegli_tools_test)
  endif()
  foreach(SCRIPT IN LISTS TEST_SCRIPTS)
    add_test(NAME ${SCRIPT}
      COMMAND ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/${SCRIPT}.sh
      ${CMAKE_BINARY_DIR})
  endforeach()
endif()  # BASH_PROGRAM
endif()  # PDFCORE_TEST_TOOLS
