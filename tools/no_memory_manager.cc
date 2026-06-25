// Copyright (c) the JPEG XL Project Authors. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "tools/no_memory_manager.h"

#include <cstdlib>

#include "lib/base/memory_manager.h"

namespace pdfcore_jpegli_tools {

namespace {
void* ToolsAlloc(void* /* opaque*/, size_t size) { return malloc(size); }
void ToolsFree(void* /* opaque*/, void* address) { free(address); }
PdfcoreMemoryManager kNoMemoryManager{nullptr, &ToolsAlloc, &ToolsFree};
}  // namespace

PdfcoreMemoryManager* NoMemoryManager() { return &kNoMemoryManager; };

}  // namespace pdfcore_jpegli_tools
