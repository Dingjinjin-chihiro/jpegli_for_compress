// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PDFCORE_LIB_BASE_OS_MACROS_H_
#define PDFCORE_LIB_BASE_OS_MACROS_H_

// Defines the PDFCORE_OS_* macros.

#if defined(_WIN32) || defined(_WIN64)
#define PDFCORE_OS_WIN 1
#else
#define PDFCORE_OS_WIN 0
#endif

#ifdef __linux__
#define PDFCORE_OS_LINUX 1
#else
#define PDFCORE_OS_LINUX 0
#endif

#ifdef __APPLE__
#define PDFCORE_OS_MAC 1
#else
#define PDFCORE_OS_MAC 0
#endif

#define PDFCORE_OS_IOS 0
#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#undef PDFCORE_OS_IOS
#define PDFCORE_OS_IOS 1
#endif
#endif

#ifdef __FreeBSD__
#define PDFCORE_OS_FREEBSD 1
#else
#define PDFCORE_OS_FREEBSD 0
#endif

#ifdef __HAIKU__
#define PDFCORE_OS_HAIKU 1
#else
#define PDFCORE_OS_HAIKU 0
#endif

#endif  // PDFCORE_LIB_BASE_OS_MACROS_H_
